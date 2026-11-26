#include "core/shader.hpp"

#include <algorithm>
#include <cmath>

#include "core/error.hpp"
#include "core/postfx.hpp"

namespace alpha::core {

namespace {

constexpr std::string_view kSourceExt = ".frag";
// Per ShaderFormat (enum order): blob file extension and name.
constexpr std::array<std::string_view, 2> kBlobExts{".spv", ".msl"};
constexpr std::array<std::string_view, 2> kFormatNames{"spirv", "msl"};
static_assert(kBlobExts.size() == kShaderFormats.size() && kFormatNames.size() == kShaderFormats.size());

std::string uniform_names(const Shader& s) {
  std::string out;
  for (const auto& [name, value] : s.uniforms) out += (out.empty() ? "" : " ") + name;
  return out.empty() ? "(none)" : out;
}

void check_finite(float value, const std::string& what) {
  if (!std::isfinite(value)) throw Error(what + " must be a finite number, got " + std::to_string(value));
}

}  // namespace

void check_uniform_count(const std::string& what, std::size_t count) {
  if (count <= kMaxShaderUniforms) return;
  throw Error(what + ": " + std::to_string(count) + " uniforms; at most " + std::to_string(kMaxShaderUniforms) + " floats");
}

Shader make_shader(std::string name, std::string file, std::vector<std::pair<std::string, float>> uniforms) {
  const std::string where = "assets.shaders." + name;
  if (effect_kind(name)) throw Error(where + ": \"" + name + "\" is a built-in postfx effect; pick another name");
  if (!file.ends_with(kSourceExt) || file.size() == kSourceExt.size()) {
    throw Error(where + ".file must be a GLSL fragment shader (.frag), got \"" + file + "\"");
  }
  check_uniform_count(where + ".uniforms", uniforms.size());
  for (const auto& [key, value] : uniforms) check_finite(value, where + ".uniforms." + key);
  return Shader{std::move(name), std::move(file), std::move(uniforms), {}};
}

std::string blob_file(std::string_view source, ShaderFormat format) {
  return std::string(source.substr(0, source.size() - kSourceExt.size())) +
         std::string(kBlobExts[static_cast<std::size_t>(format)]);
}

std::string_view format_name(ShaderFormat format) { return kFormatNames[static_cast<std::size_t>(format)]; }

std::string compiler_message(std::string_view log, std::string_view path, std::string_view file) {
  std::string out;
  std::size_t start = 0;
  while (start < log.size()) {
    const std::size_t end = std::min(log.find('\n', start), log.size());
    std::string_view line = log.substr(start, end - start);
    start = end + 1;
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    const std::size_t at = line.find(path);
    const bool shown = (line.starts_with("ERROR: ") || line.starts_with("WARNING: ")) && at != line.npos;
    if (!shown) continue;
    out += (out.empty() ? "" : "\n") + std::string(line.substr(0, at)) + std::string(file) +
           std::string(line.substr(at + path.size()));
  }
  return out.empty() ? std::string(log) : out;
}

std::optional<ShaderFormat> pick_format(std::uint32_t device_formats, const ShaderBlobs& blobs) {
  for (const ShaderFormat f : kShaderFormats) {
    if ((device_formats & format_bit(f)) != 0 && blobs.contains(f)) return f;
  }
  return std::nullopt;
}

ShaderUniforms shader_uniforms(const Shader& shader, const std::map<std::string, float, std::less<>>& numbers) {
  ShaderUniforms out{};
  for (std::size_t i = 0; i < shader.uniforms.size(); ++i) out[i] = shader.uniforms[i].second;
  for (const auto& [key, value] : numbers) {
    const auto it = std::find_if(shader.uniforms.begin(), shader.uniforms.end(), [&](const auto& u) { return u.first == key; });
    const std::string where = "postfx \"" + shader.name + "\"";
    if (it == shader.uniforms.end()) throw Error(where + ": unknown uniform \"" + key + "\"; have: " + uniform_names(shader));
    check_finite(value, where + "." + key);
    out[static_cast<std::size_t>(it - shader.uniforms.begin())] = value;
  }
  return out;
}

}  // namespace alpha::core
