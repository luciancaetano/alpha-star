#pragma once

#include <filesystem>
#include <vector>

#include "core/glyph_layout.hpp"

// assets.fonts files as font sources for world text and the game UI:
// TrueType / OpenType (rasterized per pixel size with FreeType) and AngelCode
// BMFont (.fnt, text or XML, with its PNG pages), plus the built-in font.
namespace alpha::platform {

// Loads `file` (a .fnt is a BMFont, anything else TrueType / OpenType).
// pixel_mode: crisp (monochrome TrueType, nearest filtering) or smooth.
// fallbacks (TrueType only): fonts drawing what `file` lacks, in order
// (platform/truetype). Throws core::Error naming the file and what is wrong.
core::FontSourcePtr load_font(const std::filesystem::path& file, bool pixel_mode,
                              const std::vector<core::FontSourcePtr>& fallbacks = {});
// True for a BMFont description (.fnt).
bool is_bitmap_font(const std::filesystem::path& file);
// The built-in 8x8 public domain font (ASCII + Latin-1; boxes for the rest),
// used when a game names no font. Always crisp.
core::FontSourcePtr default_font();

}  // namespace alpha::platform
