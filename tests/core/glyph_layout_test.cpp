#include <doctest/doctest.h>

#include <memory>

#include "core/glyph_layout.hpp"

using alpha::core::Align;
using alpha::core::FontAtlas;
using alpha::core::glyph_key;
using alpha::core::ShapedLine;
using alpha::core::ShapedText;
using alpha::core::Vec2;

namespace {

// Glyph 1: 6x10 at offset (1, 2); glyph 2: a space (nothing drawn).
ShapedText shaped(std::vector<ShapedLine> lines) {
  auto f = std::make_shared<FontAtlas>();
  f->line_height = 12;
  f->glyphs[glyph_key(0, 1)] = {{0, 0, 6, 10}, {1, 2}, 8};
  f->glyphs[glyph_key(0, 2)] = {{0, 0, 0, 0}, {}, 4};
  return {f, std::move(lines)};
}

ShapedLine line(float width, std::initializer_list<float> pens) {
  ShapedLine l{{}, width};
  for (const float x : pens) l.glyphs.push_back({glyph_key(0, 1), x, 0});
  return l;
}

}  // namespace

TEST_CASE("centered text: one quad per drawn glyph, box = line") {
  ShapedLine l = line(28, {0, 8, 20});
  l.glyphs.insert(l.glyphs.begin() + 2, {glyph_key(0, 2), 16, 0});  // space: no quad
  const auto q = alpha::core::layout_text(shaped({l}), Align::Center);
  CHECK(q.box == Vec2{28, 12});
  REQUIRE(q.grid.tiles.size() == 3);
  CHECK(q.grid.tiles[0].pos == Vec2{1, 2});
  CHECK(q.grid.tiles[2].pos == Vec2{21, 2});
}

TEST_CASE("left/right align: box twice the line, glyphs on the anchor's side") {
  const auto left = alpha::core::layout_text(shaped({line(16, {0, 8})}), Align::Left);
  CHECK(left.box.x == 32);
  CHECK(left.grid.tiles[0].pos.x == 17);
  const auto right = alpha::core::layout_text(shaped({line(16, {0, 8})}), Align::Right);
  CHECK(right.grid.tiles[0].pos.x == 1);
}

TEST_CASE("several lines stack by line height, each aligned in the widest") {
  const auto q = alpha::core::layout_text(shaped({line(32, {0}), line(16, {0})}), Align::Center);
  CHECK(q.box == Vec2{32, 24});
  REQUIRE(q.grid.tiles.size() == 2);
  CHECK(q.grid.tiles[1].pos == Vec2{9, 14});  // (32 - 16) / 2 + 1, 12 + 2
  const auto r = alpha::core::layout_text(shaped({line(32, {0}), line(16, {0})}), Align::Right);
  CHECK(r.grid.tiles[1].pos.x == 17);
}

TEST_CASE("a glyph the atlas lacks draws nothing") {
  ShapedLine l{{{glyph_key(3, 9), 0, 0}}, 8};
  CHECK(alpha::core::layout_text(shaped({l}), Align::Center).grid.tiles.empty());
}

TEST_CASE("font scale: scalable at the output size, bitmap at whole multiples") {
  CHECK(alpha::core::font_scale(0, 16, 2.5f).pixels == 40);
  CHECK(alpha::core::font_scale(8, 16, 1.5f).texel == 3);
  CHECK(alpha::core::font_scale(0, 300, 2).texel == doctest::Approx(600.0f / 320.0f));
}
