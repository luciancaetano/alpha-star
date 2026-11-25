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

TEST_CASE("debug font boxes scale with size") {
  CHECK(alpha::core::debug_text_box("abc", 16, Align::Center) == Vec2{48, 16});
  CHECK(alpha::core::debug_text_box("abc", 16, Align::Right) == Vec2{96, 16});
}
