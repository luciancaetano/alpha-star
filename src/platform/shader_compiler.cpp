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
constexpr const char* kTool = "shadercross.exe";
#else
constexpr const char* kTool = "shadercross";
#endif

// $ALPHA_SHADERCROSS, else the tool next to the engine.
std::filesystem::path tool() {
  if (const char* env = SDL_getenv("ALPHA_SHADERCROSS"); env && *env) return env;
  const char* base = SDL_GetBasePath();
  return std::filesystem::path(base ? base : "") / kTool;
}

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

const char* dest(core::ShaderFormat f) {
  if (f == core::ShaderFormat::Spirv) return "SPIRV";
  return f == core::ShaderFormat::Dxil ? "DXIL" : "MSL";
}

// Where one compile lands: the system temp folder, one name per source + format.
std::filesystem::path output(const std::filesystem::path& source, core::ShaderFormat f) {
  const std::size_t key = std::hash<std::string>{}(source.generic_string());
  return std::filesystem::temp_directory_path() /
         ("alpha-shader-" + std::to_string(key) + "." + std::string(core::format_name(f)));
}

core::ShaderBlobs compile_all(const std::filesystem::path& source, const std::string& what) {
  core::ShaderBlobs out;
  for (const core::ShaderFormat f : core::kShaderFormats) {
    const std::filesystem::path to = output(source, f);
    const auto [ok, log] = run({tool().string(), source.string(), "-s", "HLSL", "-d", dest(f), "-t", "fragment", "-o",
                                to.string()});
    auto bytes = ok ? vfs::read(to) : std::nullopt;
    if (!bytes || bytes->empty()) throw core::Error(what + ": " + source.filename().string() + " did not compile:\n" + log);
    out[f] = std::move(*bytes);
  }
  return out;
}

}  // namespace

bool shader_compiler_available() { return vfs::exists(tool()); }

std::string shader_compiler_hint() { return "; no shader compiler at " + tool().string(); }

core::ShaderBlobs compile_shader(const std::filesystem::path& source, const std::string& what) {
  // One compile per source edit: simulate instances and reboots reuse it.
  static std::map<std::string, std::pair<std::int64_t, core::ShaderBlobs>, std::less<>> cache;
  const std::int64_t modified = vfs::info(source).value_or(vfs::Info{}).modified;
  const auto hit = cache.find(source.generic_string());
  if (hit != cache.end() && hit->second.first == modified && modified != 0) return hit->second.second;
  core::ShaderBlobs out = compile_all(source, what);
  cache[source.generic_string()] = {modified, out};
  return out;
}

#else

bool shader_compiler_available() { return false; }

std::string shader_compiler_hint() { return ""; }

core::ShaderBlobs compile_shader(const std::filesystem::path&, const std::string& what) {
  throw core::Error(what + ": this engine only loads compiled shaders");
}

#endif

}  // namespace alpha::platform
