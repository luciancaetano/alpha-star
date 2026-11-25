#pragma once

#include <array>
#include <memory>
#include <string_view>

#include "core/entity_spec.hpp"

// World text layout (pure): a line of text as glyph quads of a font atlas,
// ready to draw through the tile path (one textured batch).
namespace alpha::core {

// Glyphs: printable ASCII (U+0020..U+007E) then Latin-1 (U+00A0..U+00FF),
// enough for most Western European languages.
inline constexpr std::size_t kAsciiGlyphs = '~' - ' ' + 1;
inline constexpr std::size_t kGlyphCount = kAsciiGlyphs + (0xFF - 0xA0 + 1);
// Code point of glyph i.
char32_t glyph_code(std::size_t i);

struct Glyph {
  Region src;      // in the atlas image (empty for a space)
  Vec2 offset;     // bitmap top-left from the pen, y from the line top
  float advance = 0.0f;
};

// The glyphs of one font at one pixel size, rasterized into `image`.
struct FontAtlas {
  ImagePtr image;
  std::array<Glyph, kGlyphCount> glyphs{};
  float line_height = 0.0f;
};
using FontAtlasPtr = std::shared_ptr<const FontAtlas>;

struct TextQuads {
  TileGrid grid;  // glyph quads, positions relative to the box's top-left
  Vec2 box;       // size of the drawn box
};

// Lays out `text` on one line. The box is centered on the entity, so for
// left/right align it is twice the line's width with the glyphs in the
// half that starts/ends at the anchor (culling stays a centered box).
// `text` is UTF-8; characters without a glyph and invalid bytes draw as '?'.
TextQuads layout_text(const FontAtlas& font, std::string_view text, Align align);
// Box of a line in the 8x8 debug font scaled to `size` (same anchor rule).
Vec2 debug_text_box(std::string_view text, float size, Align align);

}  // namespace alpha::core
