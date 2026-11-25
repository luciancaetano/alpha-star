#include <doctest/doctest.h>

#include "core/bmfont.hpp"
#include <string>

#include "core/error.hpp"

using alpha::core::parse_bmfont;

namespace {

constexpr const char* kText = R"(info face="Tiny" size=-8 bold=0 italic=0 charset="" unicode=1 stretchH=100 smooth=0 aa=1 padding=0,0,0,0 spacing=1,1
common lineHeight=10 base=8 scaleW=64 scaleH=32 pages=2 packed=0
page id=0 file="tiny_0.png"
page id=1 file="tiny_1.png"
chars count=3
char id=32   x=0  y=0  width=0  height=0  xoffset=0  yoffset=0  xadvance=4  page=0  chnl=15
char id=65   x=1  y=2  width=5  height=7  xoffset=0  yoffset=1  xadvance=6  page=0  chnl=15
char id=86   x=8  y=0  width=5  height=7  xoffset=-1 yoffset=1  xadvance=6  page=1  chnl=15
kernings count=1
kerning first=65  second=86  amount=-2
)";

constexpr const char* kXml = R"(<?xml version="1.0"?>
<font>
  <info face="Tiny" size="8" />
  <common lineHeight="10" base="8" scaleW="64" scaleH="32" pages="1" packed="0" />
  <pages>
    <page id="0" file="tiny 0.png" />
  </pages>
  <chars count="1">
    <char id="65" x="1" y="2" width="5" height="7" xoffset="0" yoffset="1" xadvance="6" page="0" chnl="15" />
  </chars>
  <kernings count="1">
    <kerning first="65" second="65" amount="1" />
  </kernings>
</font>
)";

}  // namespace

TEST_CASE("BMFont text format: metrics, pages, chars and kerning pairs") {
  const auto f = parse_bmfont(kText);
  CHECK(f.size == 8);  // negative size = cell height; the magnitude is the size
  CHECK(f.line_height == 10);
  CHECK(f.base == 8);
  REQUIRE(f.pages.size() == 2);
  CHECK(f.pages[1] == "tiny_1.png");
  REQUIRE(f.chars.size() == 3);
  CHECK(f.chars[1].id == U'A');
  CHECK(f.chars[1].src.x == 1);
  CHECK(f.chars[1].src.h == 7);
  CHECK(f.chars[2].offset.x == -1);
  CHECK(f.chars[2].page == 1);
  CHECK(f.chars[1].advance == 6);
  REQUIRE(f.kernings.size() == 1);
  CHECK(f.kernings[0].amount == -2);
}

TEST_CASE("BMFont XML format reads the same fields") {
  const auto f = parse_bmfont(kXml);
  CHECK(f.size == 8);
  REQUIRE(f.pages.size() == 1);
  CHECK(f.pages[0] == "tiny 0.png");
  REQUIRE(f.chars.size() == 1);
  CHECK(f.chars[0].offset.y == 1);
  CHECK(f.kernings.at(0).first == U'A');
}

TEST_CASE("BMFont atlas: glyphs land on stacked pages, kerning kept, errors name the problem") {
  const auto f = parse_bmfont(kText);
  const auto atlas = alpha::core::bitmap_atlas(f, {0, 32});
  CHECK(atlas.line_height == 10);
  CHECK(atlas.ascent == 8);
  const auto& v = atlas.glyphs[U'V' - U' '];
  CHECK(v.src.y == 32);  // page 1 sits under page 0
  CHECK(v.offset.x == -1);
  CHECK(atlas.kerning.at({U'A', U'V'}) == -2);
  CHECK(atlas.glyphs[U'B' - U' '].src.w == 0);  // missing: draws nothing
  CHECK_THROWS_WITH(parse_bmfont("char id=65"), doctest::Contains("common"));
  CHECK_THROWS_WITH(parse_bmfont("BMF\x03"), doctest::Contains("binary"));
  std::string bad_page = kText;
  bad_page.replace(bad_page.find("page=1"), 6, "page=7");
  CHECK_THROWS_WITH(parse_bmfont(bad_page), doctest::Contains("page 7"));
}
