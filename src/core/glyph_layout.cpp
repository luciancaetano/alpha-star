#include "core/glyph_layout.hpp"

#include <vector>

namespace alpha::core {

namespace {

// Glyph index of code point c, or the index of '?'.
std::size_t index_of(char32_t c) {
  if (c >= U' ' && c <= U'~') return c - U' ';
  if (c >= 0xA0 && c <= 0xFF) return kAsciiGlyphs + (c - 0xA0);
  return U'?' - U' ';
}

// Length of a UTF-8 sequence from its lead byte (0 = not a lead byte).
std::size_t sequence_length(unsigned char lead) {
  if (lead < 0x80) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  if ((lead & 0xF8) == 0xF0) return 4;
  return 0;
}

// Code points of UTF-8 `text`; a broken sequence gives one '?' per bad byte.
std::vector<char32_t> decode(std::string_view text) {
  std::vector<char32_t> out;
  for (std::size_t i = 0; i < text.size();) {
    const auto lead = static_cast<unsigned char>(text[i]);
    const std::size_t n = sequence_length(lead);
    bool ok = n > 0 && i + n <= text.size();
    char32_t c = n == 1 ? lead : static_cast<char32_t>(lead & (0x7F >> n));
    for (std::size_t k = 1; ok && k < n; ++k) {
      const auto b = static_cast<unsigned char>(text[i + k]);
      ok = (b & 0xC0) == 0x80;
      c = (c << 6) | (b & 0x3F);
    }
    out.push_back(ok ? c : U'?');
    i += ok ? n : 1;
  }
  return out;
}

// Box width and where the line starts in it, for a line `width` wide.
std::pair<float, float> anchored(float width, Align align) {
  if (align == Align::Center) return {width, 0.0f};
  return {width * 2.0f, align == Align::Left ? width : 0.0f};
}

}  // namespace

char32_t glyph_code(std::size_t i) {
  return i < kAsciiGlyphs ? static_cast<char32_t>(U' ' + i) : static_cast<char32_t>(0xA0 + (i - kAsciiGlyphs));
}

TextQuads layout_text(const FontAtlas& font, std::string_view text, Align align) {
  const std::vector<char32_t> codes = decode(text);
  float width = 0.0f;
  for (const char32_t c : codes) width += font.glyphs[index_of(c)].advance;
  const auto [box_width, start] = anchored(width, align);
  TextQuads out{TileGrid{}, Vec2{box_width, font.line_height}};
  float pen = start;
  for (const char32_t c : codes) {
    const Glyph& g = font.glyphs[index_of(c)];
    if (g.src.w > 0 && g.src.h > 0) out.grid.tiles.push_back({Vec2{pen + g.offset.x, g.offset.y}, g.src});
    pen += g.advance;
  }
  return out;
}

Vec2 debug_text_box(std::string_view text, float size, Align align) {
  return {anchored(static_cast<float>(text.size()) * size, align).first, size};
}

}  // namespace alpha::core
