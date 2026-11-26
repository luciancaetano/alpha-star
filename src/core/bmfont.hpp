#pragma once

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/glyph_layout.hpp"

// AngelCode BMFont descriptions (.fnt, text or XML; not the binary format):
// a bitmap font drawn at one size into PNG pages.
namespace alpha::core {

struct BmChar {
  char32_t id = 0;
  Region src;      // on its page
  Vec2 offset;     // from the pen, y from the line top
  float advance = 0.0f;
  int page = 0;
};

struct BmKerning {
  char32_t first = 0;
  char32_t second = 0;
  float amount = 0.0f;
};

struct BmFont {
  int size = 0;              // the size it was drawn at (info size, magnitude)
  float line_height = 0.0f;  // common lineHeight
  float base = 0.0f;         // common base: baseline from the line top
  std::vector<std::string> pages;  // image files, relative to the .fnt
  std::vector<BmChar> chars;
  std::vector<BmKerning> kernings;
};

// Throws core::Error naming what is missing or wrong.
BmFont parse_bmfont(std::string_view text);
// The atlas of a font whose pages are stacked in one image, page i starting
// at row page_top[i] (the image itself is left for the caller to attach);
// glyph keys are glyph_key(0, code point). Kerning pairs by code point.
struct BitmapAtlas {
  FontAtlas atlas;
  std::map<std::pair<char32_t, char32_t>, float> kerning;
};
BitmapAtlas bitmap_atlas(const BmFont& font, const std::vector<int>& page_top);

}  // namespace alpha::core
