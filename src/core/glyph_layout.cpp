#include "core/glyph_layout.hpp"

namespace alpha::core {

namespace {

const Glyph& glyph_of(const FontAtlas& font, char c) {
  if (c < kFirstGlyph || c > kLastGlyph) c = '?';
  return font.glyphs[static_cast<std::size_t>(c - kFirstGlyph)];
}

// Box width and where the line starts in it, for a line `width` wide.
std::pair<float, float> anchored(float width, Align align) {
  if (align == Align::Center) return {width, 0.0f};
  return {width * 2.0f, align == Align::Left ? width : 0.0f};
}

}  // namespace

TextQuads layout_text(const FontAtlas& font, std::string_view text, Align align) {
  float width = 0.0f;
  for (const char c : text) width += glyph_of(font, c).advance;
  const auto [box_width, start] = anchored(width, align);
  TextQuads out{TileGrid{}, Vec2{box_width, font.line_height}};
  float pen = start;
  for (const char c : text) {
    const Glyph& g = glyph_of(font, c);
    if (g.src.w > 0 && g.src.h > 0) out.grid.tiles.push_back({Vec2{pen + g.offset.x, g.offset.y}, g.src});
    pen += g.advance;
  }
  return out;
}

Vec2 debug_text_box(std::string_view text, float size, Align align) {
  return {anchored(static_cast<float>(text.size()) * size, align).first, size};
}

}  // namespace alpha::core
