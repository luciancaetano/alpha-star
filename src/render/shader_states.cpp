#include "render/shader_states.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace alpha::render {

namespace {

// SDL's shader format bits as core::format_bit bits.
std::uint32_t device_formats(SDL_GPUDevice* device) {
  const SDL_GPUShaderFormat f = SDL_GetGPUShaderFormats(device);
  const auto bit = [f](SDL_GPUShaderFormat sdl, core::ShaderFormat ours) {
    return static_cast<std::uint32_t>((f & sdl) != 0) * core::format_bit(ours);
  };
  return bit(SDL_GPU_SHADERFORMAT_SPIRV, core::ShaderFormat::Spirv) | bit(SDL_GPU_SHADERFORMAT_MSL, core::ShaderFormat::Msl);
}

// Per core::ShaderFormat (kShaderFormats order): SDL's format and the entry point
// (SPIRV-Cross renames GLSL's main to main0 in MSL).
constexpr std::array<SDL_GPUShaderFormat, 2> kSdlFormats{SDL_GPU_SHADERFORMAT_SPIRV, SDL_GPU_SHADERFORMAT_MSL};
constexpr std::array<const char*, 2> kEntryPoints{"main", "main0"};
static_assert(kSdlFormats.size() == core::kShaderFormats.size() && kEntryPoints.size() == core::kShaderFormats.size());

}  // namespace

ShaderStates::~ShaderStates() {
  for (auto& [key, e] : entries_) release(e);
}

void ShaderStates::release(Entry& e) {
  if (e.state) SDL_DestroyGPURenderState(e.state);
  if (e.shader) SDL_ReleaseGPUShader(SDL_GetGPURendererDevice(sdl_), e.shader);
  e = {};
}

void ShaderStates::warn(const core::Shader& shader, const std::string& why) {
  if (!warned_.insert(shader.name).second) return;
  SDL_Log("postfx \"%s\": %s; effect skipped", shader.name.c_str(), why.c_str());
}

ShaderStates::Entry ShaderStates::make(const core::ShaderPtr& shader) {
  Entry e{shader};
  SDL_GPUDevice* device = SDL_GetGPURendererDevice(sdl_);
  if (!device) {
    warn(*shader, std::string("custom shaders need Vulkan (SDL's GPU renderer on the vulkan driver); this renderer is \"") +
                          SDL_GetRendererName(sdl_) + "\"");
    return e;
  }
  const auto format = core::pick_format(device_formats(device), shader->blobs);
  if (!format) return warn(*shader, "this GPU takes none of the pack's shader formats (SPIR-V for Vulkan, MSL for Metal)"), e;
  return create(device, std::move(e), *format);
}

ShaderStates::Entry ShaderStates::create(SDL_GPUDevice* device, Entry e, core::ShaderFormat format) {
  const core::ShaderPtr shader = e.owner.lock();
  const auto& code = shader->blobs.at(format);
  const auto i = static_cast<std::size_t>(format);
  SDL_GPUShaderCreateInfo info{};
  info.code = code.data();
  info.code_size = code.size();
  info.entrypoint = kEntryPoints[i];
  info.format = kSdlFormats[i];
  info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  info.num_samplers = 1;
  info.num_uniform_buffers = static_cast<Uint32>(!shader->uniforms.empty());
  e.shader = SDL_CreateGPUShader(device, &info);
  SDL_GPURenderStateCreateInfo create{};
  create.fragment_shader = e.shader;
  e.state = SDL_CreateGPURenderState(sdl_, &create);
  if (!e.state) warn(*shader, std::string("GPU rejected the shader: ") + SDL_GetError());
  return e;
}

SDL_GPURenderState* ShaderStates::state(const core::Effect& effect) {
  auto it = entries_.find(effect.shader.get());
  if (it == entries_.end() || it->second.owner.expired()) {
    if (it != entries_.end()) release(it->second);
    it = entries_.insert_or_assign(effect.shader.get(), make(effect.shader)).first;
  }
  Entry& e = it->second;
  if (e.state && !effect.shader->uniforms.empty()) {
    SDL_SetGPURenderStateFragmentUniforms(e.state, 0, effect.uniforms.data(),
                                          static_cast<Uint32>(sizeof(float) * effect.uniforms.size()));
  }
  return e.state;
}

void ShaderStates::prune() {
  for (auto it = entries_.begin(); it != entries_.end();) {
    if (!it->second.owner.expired()) {
      ++it;
      continue;
    }
    release(it->second);
    it = entries_.erase(it);
  }
}

}  // namespace alpha::render
