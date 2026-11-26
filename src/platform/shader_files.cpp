#include "platform/shader_files.hpp"

#include "core/error.hpp"
#include "platform/shader_compiler.hpp"
#include "platform/vfs.hpp"

namespace alpha::platform {

namespace {

// The pack's blobs next to the source; formats without a file are left out.
core::ShaderBlobs packed_blobs(const std::filesystem::path& root, const std::string& file, const std::string& what) {
  core::ShaderBlobs out;
  for (const core::ShaderFormat f : core::kShaderFormats) {
    const std::filesystem::path path = vfs::in_pack(root, core::blob_file(file, f));
    auto bytes = vfs::read(path);
    if (!bytes) continue;
    if (bytes->size() > core::kMaxShaderBlobBytes || bytes->empty()) {
      throw core::Error(what + ": " + core::blob_file(file, f) + " is " + std::to_string(bytes->size()) +
                        " bytes; a compiled shader is 1.." + std::to_string(core::kMaxShaderBlobBytes));
    }
    out[f] = std::move(*bytes);
  }
  return out;
}

}  // namespace

core::ShaderBlobs load_shader_blobs(const std::filesystem::path& root, const std::string& file, const std::string& what) {
  const std::filesystem::path source = vfs::in_pack(root, file);
  if (shader_compiler_available() && vfs::exists(source)) return compile_shader(source, file, what);
  core::ShaderBlobs blobs = packed_blobs(root, file, what);
  if (blobs.empty()) {
    throw core::Error(what + ": no compiled shader for \"" + file + "\" (looked for " +
                      core::blob_file(file, core::ShaderFormat::Spirv) + shader_compiler_hint() +
                      "); alpha export writes it, alpha run compiles the .frag");
  }
  return blobs;
}

}  // namespace alpha::platform
