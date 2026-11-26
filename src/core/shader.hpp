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

// Custom full-screen shaders (assets.shaders): one HLSL fragment shader per
// asset, compiled by SDL_shadercross into one blob per GPU backend format.
// Data and validation only; render::ShaderStates turns them into GPU state.
namespace alpha::core {

enum class ShaderFormat { Spirv, Dxil, Msl };
inline constexpr std::array<ShaderFormat, 3> kShaderFormats{ShaderFormat::Spirv, ShaderFormat::Dxil, ShaderFormat::Msl};

using ShaderBlobs = std::map<ShaderFormat, std::vector<std::uint8_t>>;

// One assets.shaders entry. `uniforms` are the shader's float params in
// cbuffer order (register(b0, space3)) with their defaults.
struct Shader {
  std::string name;
  std::string file;  // the HLSL source, relative to the pack ("shaders/wave.hlsl")
  std::vector<std::pair<std::string, float>> uniforms;
  ShaderBlobs blobs;
};
using ShaderPtr = std::shared_ptr<const Shader>;
using ShaderUniforms = std::array<float, kMaxShaderUniforms>;

// Validated shader (blobs empty). Throws Error naming assets.shaders.<name>:
// a built-in effect's name, a file that is not .hlsl, too many uniforms or a
// non-finite default.
Shader make_shader(std::string name, std::string file, std::vector<std::pair<std::string, float>> uniforms);
// Throws Error when `count` uniforms exceed kMaxShaderUniforms (`what`: assets.shaders.<name>.uniforms).
void check_uniform_count(const std::string& what, std::size_t count);
// "shaders/wave.hlsl" -> "shaders/wave.spv" / ".dxil" / ".msl": where a pack keeps the blob.
std::string blob_file(std::string_view hlsl, ShaderFormat format);
std::string_view format_name(ShaderFormat format);  // "spirv" "dxil" "msl"
// The first format (SPIR-V, DXIL, MSL) the device takes and the shader has a blob for.
std::optional<ShaderFormat> pick_format(std::uint32_t device_formats, const ShaderBlobs& blobs);
// Bit per format in the order of kShaderFormats (what pick_format takes).
constexpr std::uint32_t format_bit(ShaderFormat f) { return 1u << static_cast<unsigned>(f); }
// The shader's uniforms: defaults, then `numbers` by name. Throws Error
// listing the shader's uniforms for an unknown name or a non-finite value.
ShaderUniforms shader_uniforms(const Shader& shader, const std::map<std::string, float, std::less<>>& numbers);

}  // namespace alpha::core
