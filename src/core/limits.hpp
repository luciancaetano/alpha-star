#pragma once

#include <cstddef>

namespace alpha::core {

// Caps on NATIVE memory a script can ask for. The JS heap limit
// (script::kMemoryLimitMb) does not cover C++ allocations sized by script
// data, so every such size is bounded here, one place per cap, and refused
// with a readable script error before anything is allocated.
// (Live particles per world are capped by kMaxParticles in core/particles.hpp.)

// Items in any array a native reads from a script (tags, numbers, map cells:
// a 1000 x 1000 tile layer fits).
inline constexpr std::size_t kMaxArrayItems = std::size_t{1} << 20;
// Entities in one scene's world, living or destroyed-but-not-yet-freed.
inline constexpr std::size_t kMaxEntities = 100000;
// Characters of displayed text (world text, UI element text).
inline constexpr std::size_t kMaxTextChars = 4096;
// Notes in one synthesized sound (each note is at most 10 s).
inline constexpr std::size_t kMaxSynthNotes = 256;
// Game instances alive at once (the game plus simulate runs; each owns a world, UI and mixer).
inline constexpr std::size_t kMaxInstances = 64;
// Custom shaders (assets.shaders): float uniforms per shader (one 64-byte cbuffer),
// shaders per game, and bytes per compiled blob read from a pack.
inline constexpr std::size_t kMaxShaderUniforms = 16;
inline constexpr std::size_t kMaxShaders = 32;
inline constexpr std::size_t kMaxShaderBlobBytes = std::size_t{4} << 20;

}  // namespace alpha::core
