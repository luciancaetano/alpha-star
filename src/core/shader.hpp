#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/limits.hpp"

// Custom full-screen shaders (assets.shaders): one Vulkan GLSL fragment shader
// (.frag) per asset, compiled by glslang to SPIR-V (Vulkan), then by SDL_shadercross
// from that SPIR-V to MSL (Metal; no DXC). ShaderFormat is the one seam for backends:
// a D3D12/Xbox format would be SPIR-V -> HLSL (shadercross, no DXC) -> the platform's
// compiler. Data and validation only; render::ShaderStates makes GPU state.
namespace alpha::core {

// kShaderFormats order is the preference when a device takes several.
enum class ShaderFormat { Spirv, Msl };
inline constexpr std::array<ShaderFormat, 2> kShaderFormats{ShaderFormat::Spirv, ShaderFormat::Msl};

using ShaderBlobs = std::map<ShaderFormat, std::vector<std::uint8_t>>;

// One assets.shaders entry. `uniforms` are the shader's float params in the
// declared order of its std140 uniform block (set = 3, binding = 0), with defaults.
struct Shader {
  std::string name;
  std::string file;  // the GLSL source, relative to the pack ("shaders/wave.frag")
  std::vector<std::pair<std::string, float>> uniforms;
  ShaderBlobs blobs;
};
using ShaderPtr = std::shared_ptr<const Shader>;
using ShaderUniforms = std::array<float, kMaxShaderUniforms>;

// Validated shader (blobs empty). Throws Error naming assets.shaders.<name>:
// a built-in effect's name, a file that is not .frag, too many uniforms or a
// non-finite default.
Shader make_shader(std::string name, std::string file, std::vector<std::pair<std::string, float>> uniforms);
// Throws Error when `count` uniforms exceed kMaxShaderUniforms (`what`: assets.shaders.<name>.uniforms).
void check_uniform_count(const std::string& what, std::size_t count);
// "shaders/wave.frag" -> "shaders/wave.spv" / ".msl": where a pack keeps the blob.
std::string blob_file(std::string_view source, ShaderFormat format);
std::string_view format_name(ShaderFormat format);  // "spirv", "msl"
// The first format the device takes and the shader has a blob for (the one place
// that chooses a backend format).
std::optional<ShaderFormat> pick_format(std::uint32_t device_formats, const ShaderBlobs& blobs);
// Bit per format in the order of kShaderFormats (what pick_format takes).
constexpr std::uint32_t format_bit(ShaderFormat f) { return 1u << static_cast<unsigned>(f); }
// The shader's uniforms: defaults, then `numbers` by name. Throws Error
// listing the shader's uniforms for an unknown name or a non-finite value.
// The compiler's log for an agent: its ERROR/WARNING lines with the compiled
// `path` shown as the pack-relative `file` ("fx/wave.frag:3: 'nope' : undeclared
// identifier"); the whole log when it has no such line.
std::string compiler_message(std::string_view log, std::string_view path, std::string_view file);
ShaderUniforms shader_uniforms(const Shader& shader, const std::map<std::string, float, std::less<>>& numbers);

}  // namespace alpha::core
