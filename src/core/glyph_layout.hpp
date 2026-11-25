#pragma once

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

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
// Glyph index of code point c; nullopt outside ASCII + Latin-1.
std::optional<std::size_t> glyph_index(char32_t c);

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
  float ascent = 0.0f;  // baseline from the line top
  // Extra pen movement between two code points (BMFont kerning pairs, TrueType kern table).
  std::map<std::pair<char32_t, char32_t>, float> kerning;
};
using FontAtlasPtr = std::shared_ptr<const FontAtlas>;

// A font file loaded once: scalable fonts rasterize an atlas per pixel size,
// bitmap fonts have one atlas at their native size.
class FontSource {
 public:
  virtual ~FontSource() = default;
  // 0 for a scalable font; the size a bitmap font was drawn at.
  virtual int native() const = 0;
  // The atlas at `pixels` (ignored by bitmap fonts). Throws core::Error.
  virtual FontAtlasPtr atlas(int pixels) const = 0;
};

// How text of `size` logical pixels is drawn where one logical pixel is
// `density` output pixels: rasterized at `pixels`, each atlas pixel covering
// `texel` x `texel` output pixels. Scalable: pixels = size * density, texel 1.
// Bitmap: its native size, texel = the nearest whole multiple (at least 1).
// Scalable fonts stop at kMaxTextPixels (atlas size); bigger text stretches it.
inline constexpr int kMaxTextPixels = 320;
struct FontScale {
  int pixels = 1;
  float texel = 1.0f;
};
FontScale font_scale(int native, float size, float density);

struct TextQuads {
  TileGrid grid;  // glyph quads, positions relative to the box's top-left
  Vec2 box;       // size of the drawn box
};

// Lays out `text` on one line. The box is centered on the entity, so for
// left/right align it is twice the line's width with the glyphs in the
// half that starts/ends at the anchor (culling stays a centered box).
// `text` is UTF-8; characters without a glyph and invalid bytes draw as '?'.
// Kerning pairs of the font apply between consecutive characters.
TextQuads layout_text(const FontAtlas& font, std::string_view text, Align align);

}  // namespace alpha::core
