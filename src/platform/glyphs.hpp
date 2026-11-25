#pragma once

#include <filesystem>

#include "core/glyph_layout.hpp"

namespace alpha::platform {

// Rasterizes printable ASCII of a TTF/OTF font at `pixels` (em size) with
// FreeType (the engine already links it for RmlUi) into one white RGBA atlas
// whose alpha is the glyph coverage, so world text tints like any image.
// Throws core::Error naming the file when it cannot be read or parsed.
core::FontAtlasPtr rasterize_font(const std::filesystem::path& file, int pixels);

}  // namespace alpha::platform
