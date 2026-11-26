#pragma once

#include <filesystem>
#include <string>

#include "core/shader.hpp"

namespace alpha::platform {

// Runtime shader compilation (debug-tools builds only; shipping engines only
// read compiled blobs): runs the shadercross tool that sits next to the engine
// (HLSL -> SPIR-V, DXIL, MSL), never linked into it.
bool shader_compiler_available();
// "" or a note for "no blobs" errors (where the missing tool was looked for).
std::string shader_compiler_hint();
// All three blobs for the HLSL fragment shader at `source`. Throws core::Error
// with `what` and the compiler's own message on a compile error.
core::ShaderBlobs compile_shader(const std::filesystem::path& source, const std::string& what);

}  // namespace alpha::platform
