#include "script/bytecode.hpp"

#include <cstdio>
#include <filesystem>

#include "core/version.hpp"
#include "platform/vfs.hpp"

#if ALPHA_DEBUG_TOOLS
#include "core/log_ring.hpp"
#include "script/vm.hpp"
#endif

#define ALPHA_STR2(x) #x
#define ALPHA_STR(x) ALPHA_STR2(x)

namespace alpha::script {

// ALPHA_BUILD_ID comes from src/CMakeLists.txt (hash of the QuickJS sources + pointer width).
core::BuildStamp this_build() {
  return {std::string(core::version()),
          ALPHA_STR(QJS_VERSION_MAJOR) "." ALPHA_STR(QJS_VERSION_MINOR) "." ALPHA_STR(QJS_VERSION_PATCH),
          ALPHA_BUILD_ID};
}

Value import_bytecode_file(Engine& engine, const std::string& path) {
  const auto bytes = platform::vfs::read(path);
  if (!bytes) throw core::Error("cannot read " + path);
  const std::string_view file(reinterpret_cast<const char*>(bytes->data()), bytes->size());
  return engine.import_bytecode(core::open_bytecode(file, this_build()));
}

#if ALPHA_DEBUG_TOOLS
int compile_file(const std::string& source, const std::string& out) {
  core::LogRing log(16);
  Vm vm([](const std::string&) { return std::string(); }, log);  // the builtins "alpha", ... resolve
  try {
    const std::string module = std::filesystem::path(source).filename().string();  // no build paths inside
    platform::vfs::write_atomic(out, core::seal_bytecode(vm.engine->bytecode_of(source, module), this_build()),
                                platform::vfs::Backup::none);
    return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "compile %s failed: %s\n", source.c_str(), e.what());
    return 1;
  }
}
#endif

}  // namespace alpha::script
