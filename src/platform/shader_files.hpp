#pragma once

#include <filesystem>
#include <string>

#include "core/shader.hpp"

namespace alpha::platform {

// The compiled blobs for a shader's `file` (pack-relative .frag) under `root`.
// Debug-tools builds compile the source when it is there and the glslang
// tool sits next to the engine (so every boot and hot reload sees the edit);
// otherwise, and always in shipping, the pack's .spv file next
// to the source are read (alpha export writes them). Throws core::Error naming
// `what` (assets.shaders.<name>), the files looked for and any compiler output.
core::ShaderBlobs load_shader_blobs(const std::filesystem::path& root, const std::string& file, const std::string& what);

}  // namespace alpha::platform
