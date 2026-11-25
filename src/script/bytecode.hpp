#pragma once

#include <string>

#include "core/bytecode.hpp"
#include "script/js_engine.hpp"

namespace alpha::script {

// This engine's stamp: what it writes into game.jsbc and the only one it reads.
core::BuildStamp this_build();

// Reads `path` (a pack's game.jsbc), checks its header against this_build()
// and evaluates the module; returns its namespace. Throws core::Error.
Value import_bytecode_file(Engine& engine, const std::string& path);

#if ALPHA_DEBUG_TOOLS
// alpha_star --compile <main.js> <out.jsbc>: the bundle as stripped bytecode
// (no source, no debug info), sealed with this_build(). 0 = written, 1 = error
// (on stderr; nothing written).
int compile_file(const std::string& source, const std::string& out);
#endif

}  // namespace alpha::script
