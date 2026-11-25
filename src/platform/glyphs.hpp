#pragma once

#include <filesystem>

#include "core/glyph_layout.hpp"
#include "platform/vfs.hpp"

namespace alpha::platform {

// Rasterizes the atlas glyphs (ASCII + Latin-1) of a TTF/OTF font held in
// `bytes` at `pixels` (em size) with FreeType, hinted, into one white RGBA
// atlas whose alpha is the glyph coverage, so world text tints like any image.
// `crisp` (pixelMode) renders monochrome: every pixel fully on or off.
// Glyph offsets, advances and the ascent are whole pixels. Throws core::Error naming `file` when it cannot be parsed.
core::FontAtlasPtr rasterize_font(const vfs::Bytes& bytes, const std::filesystem::path& file, int pixels, bool crisp);

}  // namespace alpha::platform
