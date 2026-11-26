#pragma once

#include <filesystem>
#include <string>

#include "core/shader.hpp"

namespace alpha::platform {

// Runtime shader compilation (debug-tools builds only; shipping engines only
// read compiled blobs): runs the tools that sit next to the engine, never linked
// into it: glslang (Vulkan GLSL fragment shader -> SPIR-V), then shadercross
// (SPIR-V -> MSL; skipped when absent). $ALPHA_GLSLANG / $ALPHA_SHADERCROSS override them.
bool shader_compiler_available();
// "" or a note for "no blobs" errors (where the missing tool was looked for).
std::string shader_compiler_hint();
// The blobs for the GLSL fragment shader at `source` (`file`: its pack-relative
// name). Throws core::Error with `what` and the compiler's errors as
// "<file>:<line>: <message>" on a compile error.
core::ShaderBlobs compile_shader(const std::filesystem::path& source, const std::string& file, const std::string& what);

}  // namespace alpha::platform
