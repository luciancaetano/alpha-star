#include <doctest/doctest.h>

#include <vector>

#include "platform/glyph_atlas.hpp"

using alpha::platform::GlyphAtlas;
using alpha::platform::GlyphBitmap;

namespace {

// Every glyph a solid 900 x 900 square: four fit in one 2048 x 2048 atlas.
GlyphBitmap big(alpha::core::GlyphKey) {
  return {900, 900, std::vector<std::uint8_t>(900 * 900, 255), {0, 0}, 900};
}

}  // namespace

TEST_CASE("glyph atlas: grows on demand, starts over with the current text's glyphs when full, never past its cap") {
  GlyphAtlas atlas({20, 16, 8}, alpha::core::Filter::linear);
  bool dropped = true;
  const auto first = atlas.ensure({1, 2, 3, 4}, big, dropped);
  CHECK_FALSE(dropped);
  CHECK(first->glyphs.size() == 4);
  CHECK(atlas.ensure({1, 2}, big, dropped) == first);  // nothing new: the same atlas
  const auto next = atlas.ensure({5, 1}, big, dropped);  // full: only 5 and 1 remain
  CHECK_FALSE(dropped);
  CHECK(next->glyphs.size() == 2);
  CHECK(next->glyphs.contains(5));
  CHECK(next->image->height <= 2048);
  CHECK(first->glyphs.size() == 4);  // an atlas handed out never changes
  atlas.ensure({1, 2, 3, 4, 5}, big, dropped);  // more than a fresh atlas holds
  CHECK(dropped);
}
