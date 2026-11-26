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
// Bytes of displayed text (world text, UI element text), valid UTF-8.
inline constexpr std::size_t kMaxTextChars = 4096;
// Glyph atlas of one font at one pixel size: square side it may grow to (RGBA),
// and pixel sizes kept per font (least recently used dropped); a full atlas
// starts over with the glyphs the current text needs (large CJK sets).
inline constexpr int kMaxGlyphAtlasSide = 2048;
inline constexpr std::size_t kGlyphAtlasSizes = 6;
// Fallback fonts one font may list (assets.fonts.<name>.fallbacks).
inline constexpr std::size_t kMaxFontFallbacks = 8;
// Notes in one synthesized sound (each note is at most 10 s).
inline constexpr std::size_t kMaxSynthNotes = 256;
// Game instances alive at once (the game plus simulate runs; each owns a world, UI and mixer).
inline constexpr std::size_t kMaxInstances = 64;
// Custom shaders (assets.shaders): float uniforms per shader (one 64-byte std140 block),
// shaders per game, and bytes per compiled blob read from a pack.
inline constexpr std::size_t kMaxShaderUniforms = 16;
inline constexpr std::size_t kMaxShaders = 32;
inline constexpr std::size_t kMaxShaderBlobBytes = std::size_t{4} << 20;
// Touch controls (config.touchSticks / config.touchButtons) and fingers tracked at once.
inline constexpr std::size_t kMaxTouchSticks = 4;
inline constexpr std::size_t kMaxTouchButtons = 12;
inline constexpr std::size_t kMaxTouches = 10;
// alpha:net: sockets not yet closed per game instance, bytes per TCP/WS message,
// per UDP datagram (fits any path MTU), per socket inbound or outbound queue,
// and of extra WebSocket handshake headers. Socket options may lower them, never raise.
inline constexpr std::size_t kMaxNetSockets = 32;
inline constexpr std::size_t kMaxNetMessageBytes = std::size_t{1} << 20;
inline constexpr std::size_t kMaxUdpDatagramBytes = 1200;
inline constexpr std::size_t kMaxNetQueueBytes = std::size_t{8} << 20;
inline constexpr std::size_t kMaxNetHeaderBytes = std::size_t{8} << 10;

}  // namespace alpha::core
