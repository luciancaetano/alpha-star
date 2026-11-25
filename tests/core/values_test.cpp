#include <doctest/doctest.h>

#include "core/color.hpp"
#include "core/random.hpp"
#include "core/vec2.hpp"

using alpha::core::Color;
using alpha::core::parse_color;
using alpha::core::Random;
using alpha::core::Vec2;

TEST_CASE("parse_color accepts #rgb, #rrggbb and #rrggbbaa") {
  CHECK(parse_color("#f80") == Color{255, 136, 0, 255});
  CHECK(parse_color("#73bf2e") == Color{0x73, 0xbf, 0x2e, 255});
  CHECK(parse_color("#000000AA") == Color{0, 0, 0, 0xaa});
}

TEST_CASE("parse_color rejects malformed input") {
  CHECK_FALSE(parse_color(""));
  CHECK_FALSE(parse_color("73bf2e"));
  CHECK_FALSE(parse_color("#73bf2"));
  CHECK_FALSE(parse_color("#73bg2e"));
}

TEST_CASE("Vec2 arithmetic") {
  CHECK(Vec2{1, 2} + Vec2{3, 4} == Vec2{4, 6});
  CHECK(Vec2{3, 4} - Vec2{1, 2} == Vec2{2, 2});
  CHECK(Vec2{1, 2} * 3.0f == Vec2{3, 6});
}

TEST_CASE("Random is deterministic per seed") {
  Random a(42);
  Random b(42);
  Random c(43);
  bool differs = false;
  for (int i = 0; i < 16; ++i) {
    const auto va = a.next();
    CHECK(va == b.next());
    differs = differs || va != c.next();
  }
  CHECK(differs);
}

TEST_CASE("Random range and integer stay in bounds and cover them") {
  Random r(7);
  bool low = false;
  bool high = false;
  for (int i = 0; i < 2000; ++i) {
    const float f = r.range(10.0f, 20.0f);
    CHECK(f >= 10.0f);
    CHECK(f < 20.0f);
    const int n = r.integer(-2, 2);
    CHECK(n >= -2);
    CHECK(n <= 2);
    low = low || n == -2;
    high = high || n == 2;
  }
  CHECK(low);
  CHECK(high);
}
