#include "platform/shader_compiler.hpp"

#include "core/error.hpp"

#if ALPHA_DEBUG_TOOLS
#include <SDL3/SDL.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "platform/vfs.hpp"
#endif

namespace alpha::platform {

#if ALPHA_DEBUG_TOOLS

namespace {

#ifdef _WIN32
constexpr const char* kExe = ".exe";
#else
constexpr const char* kExe = "";
#endif

// $<env>, else the tool `name` next to the engine.
std::filesystem::path tool_at(const char* env, const char* name) {
  if (const char* value = SDL_getenv(env); value && *value) return value;
  const char* base = SDL_GetBasePath();
  return std::filesystem::path(base ? base : "") / (std::string(name) + kExe);
}

// glslang: GLSL -> SPIR-V. shadercross: that SPIR-V -> MSL (SPIRV-Cross; built without DXC).
std::filesystem::path tool() { return tool_at("ALPHA_GLSLANG", "glslang"); }
std::filesystem::path cross_tool() { return tool_at("ALPHA_SHADERCROSS", "shadercross"); }

// Runs `args`; its stdout + stderr, and whether it exited 0.
std::pair<bool, std::string> run(const std::vector<std::string>& args) {
  std::vector<const char*> argv;
  for (const std::string& a : args) argv.push_back(a.c_str());
  argv.push_back(nullptr);
  const SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, argv.data());
  SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
  SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
  SDL_Process* process = SDL_CreateProcessWithProperties(props);
  SDL_DestroyProperties(props);
  if (!process) return {false, std::string("cannot start ") + args[0] + ": " + SDL_GetError()};
  std::size_t size = 0;
  int code = -1;
  char* text = static_cast<char*>(SDL_ReadProcess(process, &size, &code));
  std::string out(text ? text : "", text ? size : 0);
  SDL_free(text);
  SDL_DestroyProcess(process);
  return {code == 0, out};
}

// Where one compile lands: the system temp folder, one name per source + format.
std::filesystem::path output(const std::filesystem::path& source, core::ShaderFormat f) {
  const std::size_t key = std::hash<std::string>{}(source.generic_string());
  return std::filesystem::temp_directory_path() /
         ("alpha-shader-" + std::to_string(key) + "." + std::string(core::format_name(f)));
}

// The command that writes format `f` to `to`: SPIR-V from the source, every other format
// from the SPIR-V (kShaderFormats lists SPIR-V first, so it is already there).
std::vector<std::string> command(core::ShaderFormat f, const std::filesystem::path& source, const std::filesystem::path& to) {
  if (f == core::ShaderFormat::Spirv) {
    return {tool().string(), "-V", "--target-env", "vulkan1.0", "-S", "frag", "-e", "main", "-o", to.string(), source.string()};
  }
  return {cross_tool().string(), output(source, core::ShaderFormat::Spirv).string(), "-s", "SPIRV", "-d",
          std::string(core::format_name(f)), "-t", "fragment", "-o", to.string()};
}

core::ShaderBlobs compile_all(const std::filesystem::path& source, const std::string& file, const std::string& what) {
  core::ShaderBlobs out;
  for (const core::ShaderFormat f : core::kShaderFormats) {
    // An engine built without shadercross (ALPHA_SHADERCROSS=OFF) still runs SPIR-V.
    if (f != core::ShaderFormat::Spirv && !vfs::exists(cross_tool())) continue;
    const std::filesystem::path to = output(source, f);
    const auto [ok, log] = run(command(f, source, to));
    auto bytes = ok ? vfs::read(to) : std::nullopt;
    if (!bytes || bytes->empty()) {
      throw core::Error(what + ": " + file + " did not compile:\n" + core::compiler_message(log, source.string(), file));
    }
    out[f] = std::move(*bytes);
  }
  return out;
}

}  // namespace

bool shader_compiler_available() { return vfs::exists(tool()); }

std::string shader_compiler_hint() { return "; no shader compiler at " + tool().string(); }

core::ShaderBlobs compile_shader(const std::filesystem::path& source, const std::string& file, const std::string& what) {
  // One compile per source edit and tool set: simulate instances and reboots reuse it; a
  // compile made without shadercross (SPIR-V only) is not reused once it is there.
  static std::map<std::string, std::pair<std::int64_t, core::ShaderBlobs>, std::less<>> cache;
  const std::int64_t modified = vfs::info(source).value_or(vfs::Info{}).modified;
  const std::string key = source.generic_string() + "|" + tool().string() + "|" +
                          (vfs::exists(cross_tool()) ? cross_tool().string() : std::string());
  const auto hit = cache.find(key);
  if (hit != cache.end() && hit->second.first == modified && modified != 0) return hit->second.second;
  core::ShaderBlobs out = compile_all(source, file, what);
  cache[key] = {modified, out};
  return out;
}

#else

bool shader_compiler_available() { return false; }

std::string shader_compiler_hint() { return ""; }

core::ShaderBlobs compile_shader(const std::filesystem::path&, const std::string&, const std::string& what) {
  throw core::Error(what + ": this engine only loads compiled shaders");
}

#endif

}  // namespace alpha::platform
