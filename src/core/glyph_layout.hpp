#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/entity_spec.hpp"

// Text as glyph quads (pure): a font source shapes text into positioned
// glyphs of one atlas (platform/fonts), layout_text turns them into quads
// ready to draw through the tile path (one textured batch). World text and
// the game UI share it.
namespace alpha::core {

// A glyph of a font atlas: which font of the source's chain (0 = the font
// itself, then its fallbacks in order) and the glyph id in that font (bitmap
// fonts: the code point). kTofuKey draws a box for what no font covers.
using GlyphKey = std::uint64_t;
constexpr GlyphKey glyph_key(std::uint32_t font, std::uint32_t id) { return (GlyphKey{font} << 32) | id; }
inline constexpr GlyphKey kTofuKey = ~GlyphKey{0};

struct Glyph {
  Region src;      // in the atlas image (empty for a space)
  Vec2 offset;     // bitmap top-left from the pen, y from the line top
  float advance = 0.0f;
};

// The glyphs of one font source at one pixel size, rasterized into `image`.
struct FontAtlas {
  ImagePtr image;
  std::unordered_map<GlyphKey, Glyph> glyphs;
  float line_height = 0.0f;
  float ascent = 0.0f;    // baseline from the line top
  float x_height = 0.0f;  // height of a lowercase x
};
using FontAtlasPtr = std::shared_ptr<const FontAtlas>;

// One glyph placed on its line: pen position from the line's left end (x) and
// the shaper's vertical offset (y, down), both in atlas pixels.
struct PlacedGlyph {
  GlyphKey key = 0;
  float x = 0.0f;
  float y = 0.0f;
};

// A line in visual (left-to-right drawing) order.
struct ShapedLine {
  std::vector<PlacedGlyph> glyphs;
  float width = 0.0f;
};

// Text shaped at one pixel size: its lines and the atlas holding every glyph
// they use (valid until the source shapes text again).
struct ShapedText {
  FontAtlasPtr atlas;
  std::vector<ShapedLine> lines;
  float width() const;
};

// What to shape: UTF-8 `text` (valid; checked where it entered), at `pixels`
// (ignored by bitmap fonts), wrapped at `wrap` atlas pixels (0 = only at '\n').
struct TextRequest {
  std::string_view text;
  int pixels = 16;
  TextDir dir = TextDir::Auto;
  float wrap = 0.0f;
};

// A font file loaded once: scalable fonts shape and rasterize per pixel size
// (with their fallback fonts), bitmap fonts draw at their native size.
class FontSource {
 public:
  virtual ~FontSource() = default;
  // 0 for a scalable font; the size a bitmap font was drawn at.
  virtual int native() const = 0;
  // Throws core::Error. Single-threaded: it caches atlases and shapes.
  virtual ShapedText shape(const TextRequest& request) const = 0;
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

// The lines stacked from the top, each aligned in the widest line's width.
// The box is centered on the entity, so for left/right align it is twice the
// widest line with the lines in the half that starts/ends at the anchor
// (culling stays a centered box). Glyphs missing from the atlas are skipped.
TextQuads layout_text(const ShapedText& text, Align align);

}  // namespace alpha::core
