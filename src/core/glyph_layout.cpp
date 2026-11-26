#include "core/glyph_layout.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace alpha::core {

namespace {

// Box width and where a line `width` wide starts in it, for lines at most `widest` wide.
std::pair<float, float> anchored(float widest, float width, Align align) {
  if (align == Align::Center) return {widest, (widest - width) * 0.5f};
  if (align == Align::Left) return {widest * 2.0f, widest};
  return {widest * 2.0f, widest - width};
}

}  // namespace

float ShapedText::width() const {
  float w = 0.0f;
  for (const ShapedLine& l : lines) w = std::max(w, l.width);
  return w;
}

TextQuads layout_text(const ShapedText& text, Align align) {
  const FontAtlas& font = *text.atlas;
  const float widest = text.width();
  const auto rows = static_cast<float>(std::max<std::size_t>(1, text.lines.size()));
  TextQuads out{TileGrid{}, Vec2{anchored(widest, 0.0f, align).first, font.line_height * rows}};
  for (std::size_t row = 0; row < text.lines.size(); ++row) {
    const ShapedLine& line = text.lines[row];
    const float start = std::round(anchored(widest, line.width, align).second);
    const float top = font.line_height * static_cast<float>(row);
    for (const PlacedGlyph& p : line.glyphs) {
      const auto it = font.glyphs.find(p.key);
      if (it == font.glyphs.end() || it->second.src.w <= 0 || it->second.src.h <= 0) continue;
      const Glyph& g = it->second;
      out.grid.tiles.push_back({Vec2{start + p.x + g.offset.x, top + p.y + g.offset.y}, g.src});
    }
  }
  return out;
}

FontScale font_scale(int native, float size, float density) {
  const float out = size * density;
  if (native <= 0 && out > static_cast<float>(kMaxTextPixels)) return {kMaxTextPixels, out / static_cast<float>(kMaxTextPixels)};
  if (native <= 0) return {std::max(1, static_cast<int>(std::lround(out))), 1.0f};
  return {native, std::max(1.0f, std::round(out / static_cast<float>(native)))};
}

}  // namespace alpha::core
