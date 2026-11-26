#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "core/glyph_layout.hpp"
#include "platform/fonts.hpp"
#include "platform/text_shaper.hpp"

using alpha::core::FontSourcePtr;
using alpha::core::glyph_key;
using alpha::core::GlyphKey;
using alpha::core::ShapedText;
using alpha::core::TextDir;

namespace {

const char* const kLatin = ALPHA_TEST_FIXTURES "/fonts/noto/NotoSans-Latin.ttf";
const char* const kArabic = ALPHA_TEST_FIXTURES "/fonts/noto/NotoSansArabic-Test.ttf";
const char* const kCjk = ALPHA_TEST_FIXTURES "/fonts/noto/NotoSansSC-Test.otf";
const char* const kSalam = "\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85";                    // سلام
const char* const kNihao = "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c";    // 你好世界

std::vector<GlyphKey> keys(const ShapedText& t) {
  std::vector<GlyphKey> out;
  for (const auto& g : t.lines.at(0).glyphs) out.push_back(g.key);
  return out;
}

std::uint64_t font_of(GlyphKey k) { return k >> 32; }

// Captures text warnings while alive.
struct Warnings {
  std::vector<std::string> seen;
  Warnings() {
    alpha::platform::set_text_warnings([this](const std::string& m) { seen.push_back(m); });
  }
  ~Warnings() { alpha::platform::set_text_warnings({}); }
  Warnings(const Warnings&) = delete;
  Warnings& operator=(const Warnings&) = delete;
};

}  // namespace

TEST_CASE("HarfBuzz shapes a Latin ligature: fi is one glyph, advances in pixels") {
  const FontSourcePtr latin = alpha::platform::load_font(kLatin, false);
  const ShapedText t = latin->shape({"fio", 100});
  CHECK(keys(t) == std::vector<GlyphKey>{glyph_key(0, 40), glyph_key(0, 20)});  // the fi ligature, o
  CHECK(t.lines[0].glyphs[1].x == doctest::Approx(60).epsilon(0.05));  // fi advances 0.602 em
  CHECK(t.lines[0].width == doctest::Approx(121).epsilon(0.05));       // o adds 0.605 em
  CHECK(t.atlas->glyphs.contains(glyph_key(0, 40)));
}

TEST_CASE("Arabic joins and runs right to left: contextual forms in visual order") {
  const FontSourcePtr arabic = alpha::platform::load_font(kArabic, false);
  const ShapedText t = arabic->shape({kSalam, 40});
  // Left to right: isolated meem (nominal 26), final alef (nominal 2), medial lam (20), initial seen (16).
  CHECK(keys(t) == std::vector<GlyphKey>{glyph_key(0, 26), glyph_key(0, 5), glyph_key(0, 23), glyph_key(0, 19)});
  CHECK(t.lines[0].glyphs[0].x == 0);
  CHECK(t.lines[0].glyphs[3].x > t.lines[0].glyphs[2].x);
}

TEST_CASE("bidi: a mixed line keeps each run's order and puts runs in visual order") {
  const FontSourcePtr latin = alpha::platform::load_font(kLatin, false, {alpha::platform::load_font(kArabic, false)});
  const std::string mixed = std::string("AV ") + kSalam;
  const auto ltr = keys(latin->shape({mixed, 20}));  // first strong letter is Latin: left to right
  REQUIRE(ltr.size() == 7);
  CHECK(font_of(ltr[0]) == 0);
  CHECK(ltr[6] == glyph_key(1, 19));  // the Arabic run from the fallback, its first letter rightmost
  const auto rtl = keys(latin->shape({mixed, 20, TextDir::Rtl}));  // forced: the Arabic run is leftmost
  REQUIRE(rtl.size() == 7);
  CHECK(font_of(rtl[0]) == 1);
  CHECK(font_of(rtl[6]) == 0);
  const auto auto_rtl = keys(latin->shape({std::string(kSalam) + " AV", 20}));  // Arabic first: right to left
  CHECK(font_of(auto_rtl[0]) == 0);
  CHECK(font_of(auto_rtl.back()) == 1);
}

TEST_CASE("fallback: a CJK code point comes from the second fallback; nothing covers it: a box and one warning") {
  Warnings warnings;
  const FontSourcePtr cjk = alpha::platform::load_font(kCjk, false);
  const FontSourcePtr main = alpha::platform::load_font(kLatin, false, {alpha::platform::load_font(kArabic, false), cjk});
  const auto k = keys(main->shape({"A\xe4\xbd\xa0", 24}));  // A你
  REQUIRE(k.size() == 2);
  CHECK(font_of(k[0]) == 0);
  CHECK(font_of(k[1]) == 2);
  CHECK(warnings.seen.empty());
  const ShapedText thai = main->shape({"\xe0\xb8\x81\xe0\xb8\x82", 24});  // two Thai letters: no font has them
  CHECK(keys(thai) == std::vector<GlyphKey>{alpha::core::kTofuKey, alpha::core::kTofuKey});
  CHECK(thai.atlas->glyphs.at(alpha::core::kTofuKey).src.w > 0);
  REQUIRE(warnings.seen.size() == 1);  // once per Unicode block
  CHECK(warnings.seen[0].find("U+0E01") != std::string::npos);
  CHECK(warnings.seen[0].find("fallbacks") != std::string::npos);
}

TEST_CASE("bitmap fonts: kerning pairs apply, missing glyphs draw the box, no fallbacks") {
  Warnings warnings;
  const FontSourcePtr tiny = alpha::platform::load_font(ALPHA_TEST_FIXTURES "/fonts/tiny.fnt", true);
  CHECK(tiny->shape({"AV", 99}).lines[0].glyphs[1].x == 3);  // advance 4, kerning -1
  CHECK(keys(tiny->shape({"A\xd8\xb3", 4}))[1] == alpha::core::kTofuKey);
  CHECK(warnings.seen.size() == 1);
  CHECK_THROWS_WITH(alpha::platform::load_font(ALPHA_TEST_FIXTURES "/fonts/tiny.fnt", true, {tiny}),
                    doctest::Contains("fallbacks need a .ttf/.otf"));
  CHECK(keys(alpha::platform::default_font()->shape({"\xe4\xbd\xa0", 8}))[0] == alpha::core::kTofuKey);
}

TEST_CASE("wrapping: CJK breaks between ideographs at the wrap width") {
  const FontSourcePtr cjk = alpha::platform::load_font(kCjk, false);
  const ShapedText t = cjk->shape({kNihao, 20, TextDir::Auto, 45});
  REQUIRE(t.lines.size() == 2);  // 20 px ideographs: two per 45 px line
  CHECK(t.lines[0].glyphs.size() == 2);
  CHECK(t.lines[0].width <= 45);
}

TEST_CASE("a big CJK set at a big size starts the atlas over, within its cap") {
  const FontSourcePtr cjk = alpha::platform::load_font(kCjk, false);
  const auto first = cjk->shape({"\xe4\xbd\xa0\xe5\xa5\xbd", 300}).atlas;
  const auto next = cjk->shape({std::string(kNihao) + "\xe4\xb8\xad\xe6\x96\x87", 300}).atlas;
  CHECK(next->image->width <= 2048);
  CHECK(next->image->height <= 2048);
  CHECK(next->glyphs.size() >= 6);
  CHECK(first->glyphs.size() == 2);  // an atlas handed out never changes
}
