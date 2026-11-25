#include <doctest/doctest.h>

#include "core/glyph_layout.hpp"

using alpha::core::Align;
using alpha::core::FontAtlas;
using alpha::core::Vec2;

namespace {

// Every glyph 6 px wide advancing 8, except the space (nothing drawn).
FontAtlas fake_font() {
  FontAtlas f;
  f.line_height = 12;
  for (std::size_t i = 0; i < f.glyphs.size(); ++i) {
    f.glyphs[i] = {{static_cast<int>(i) * 6, 0, 6, 10}, {1, 2}, 8};
  }
  f.glyphs[0] = {{0, 0, 0, 0}, {}, 4};
  return f;
}

}  // namespace

TEST_CASE("centered text: one quad per visible glyph, box = line") {
  const auto q = alpha::core::layout_text(fake_font(), "+1 a", Align::Center);
  CHECK(q.box == Vec2{28, 12});
  REQUIRE(q.grid.tiles.size() == 3);
  CHECK(q.grid.tiles[0].pos == Vec2{1, 2});
  CHECK(q.grid.tiles[1].pos == Vec2{9, 2});
  CHECK(q.grid.tiles[2].pos == Vec2{21, 2});  // after the 4 px space
  CHECK(q.grid.tiles[0].src.x == ('+' - ' ') * 6);
}

TEST_CASE("left/right text: doubled box, glyphs in the half at the anchor") {
  const auto left = alpha::core::layout_text(fake_font(), "ab", Align::Left);
  CHECK(left.box == Vec2{32, 12});
  CHECK(left.grid.tiles[0].pos.x == 17.0f);
  const auto right = alpha::core::layout_text(fake_font(), "ab", Align::Right);
  CHECK(right.grid.tiles[0].pos.x == 1.0f);
}

TEST_CASE("UTF-8 text: Latin-1 letters have glyphs; anything else and broken bytes draw as '?'") {
  const auto q = alpha::core::layout_text(fake_font(), "\xc3\xa7\xc3\xa3o", Align::Center);  // "ção"
  REQUIRE(q.grid.tiles.size() == 3);
  CHECK(q.grid.tiles[0].src.x == (95 + (0xE7 - 0xA0)) * 6);
  CHECK(q.grid.tiles[1].src.x == (95 + (0xE3 - 0xA0)) * 6);
  CHECK(q.grid.tiles[2].src.x == ('o' - ' ') * 6);
  CHECK(q.box.x == 24.0f);  // three glyphs, not five bytes
  const auto bad = alpha::core::layout_text(fake_font(), "\xc3 \xe2\x82\xac", Align::Center);  // lone lead byte, space, euro
  REQUIRE(bad.grid.tiles.size() == 2);
  CHECK(bad.grid.tiles[0].src.x == ('?' - ' ') * 6);
  CHECK(bad.grid.tiles[1].src.x == ('?' - ' ') * 6);
  CHECK(alpha::core::glyph_code(0) == U' ');
  CHECK(alpha::core::glyph_code(95) == 0xA0);
  CHECK(alpha::core::glyph_code(alpha::core::kGlyphCount - 1) == 0xFF);
}


TEST_CASE("kerning pairs move the pen between the two glyphs") {
  FontAtlas f = fake_font();
  f.kerning[{U'A', U'V'}] = -3;
  const auto q = alpha::core::layout_text(f, "AVA", Align::Center);
  REQUIRE(q.grid.tiles.size() == 3);
  CHECK(q.grid.tiles[1].pos.x == 1 + 8 - 3);
  CHECK(q.grid.tiles[2].pos.x == 1 + 16 - 3);  // "VA" has no pair
  CHECK(q.box.x == 21.0f);
}

TEST_CASE("font scale: scalable fonts rasterize at the output size, bitmaps draw whole texels") {
  using alpha::core::font_scale;
  CHECK(font_scale(0, 12, 1.5f).pixels == 18);
  CHECK(font_scale(0, 12, 1.5f).texel == 1);
  CHECK(font_scale(0, 0.2f, 1.0f).pixels == 1);
  CHECK(font_scale(8, 12, 1.5f).pixels == 8);
  CHECK(font_scale(8, 12, 1.5f).texel == 2);  // 18 output px / 8 = 2.25 -> 2
  CHECK(font_scale(8, 2, 1.0f).texel == 1);   // never below the native size
  CHECK(font_scale(8, 16, 3.0f).texel == 6);
  CHECK(font_scale(0, 256, 2.0f).pixels == alpha::core::kMaxTextPixels);  // huge: the biggest atlas, stretched
  CHECK(font_scale(0, 256, 2.0f).texel == doctest::Approx(512.0f / alpha::core::kMaxTextPixels));
}
