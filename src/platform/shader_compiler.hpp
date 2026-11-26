#pragma once

#include <filesystem>
#include <string>

#include "core/shader.hpp"

namespace alpha::platform {

// Runtime shader compilation (debug-tools builds only; shipping engines only
// read compiled blobs): runs glslang's standalone compiler that sits next to the
// engine (Vulkan GLSL fragment shader -> SPIR-V), never linked into it.
bool shader_compiler_available();
// "" or a note for "no blobs" errors (where the missing tool was looked for).
std::string shader_compiler_hint();
// The blobs for the GLSL fragment shader at `source` (`file`: its pack-relative
// name). Throws core::Error with `what` and the compiler's errors as
// "<file>:<line>: <message>" on a compile error.
core::ShaderBlobs compile_shader(const std::filesystem::path& source, const std::string& file, const std::string& what);

}  // namespace alpha::platform
