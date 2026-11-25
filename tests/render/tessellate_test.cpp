#include <doctest/doctest.h>

#include <vector>

#include "render/tessellate.hpp"
#include "render/text_layout.hpp"

using namespace alpha::core;
using alpha::render::kCircleSegments;
using alpha::render::place_text;
using alpha::render::tessellate;
using alpha::render::Vertex;

namespace {
const Color kRed{255, 0, 0, 255};
const Color kBlack{0, 0, 0, 255};
}  // namespace

TEST_CASE("rect becomes two triangles at its rotated corners") {
  const std::vector<DrawItem> items{{{Rect{{20, 10}}, kRed, {}}, {100, 50}, 90.0f}};
  const auto v = tessellate(items);
  REQUIRE(v.size() == 6);
  // 90 degrees: the 20x10 rect now spans 10 wide, 20 tall around (100, 50).
  float min_x = 1e9f, max_x = -1e9f, min_y = 1e9f, max_y = -1e9f;
  for (const Vertex& p : v) {
    min_x = std::min(min_x, p.pos.x);
    max_x = std::max(max_x, p.pos.x);
    min_y = std::min(min_y, p.pos.y);
    max_y = std::max(max_y, p.pos.y);
    CHECK(p.color == kRed);
  }
  CHECK(max_x - min_x == doctest::Approx(10));
  CHECK(max_y - min_y == doctest::Approx(20));
  CHECK((min_x + max_x) / 2 == doctest::Approx(100));
}

TEST_CASE("circle is a fan with every rim vertex at the radius") {
  const std::vector<DrawItem> items{{{Circle{12}, kRed, {}}, {10, 20}, 0.0f}};
  const auto v = tessellate(items);
  REQUIRE(v.size() == static_cast<std::size_t>(kCircleSegments) * 3);
  for (std::size_t i = 0; i < v.size(); i += 3) {
    CHECK(v[i].pos == Vec2{10, 20});
    const Vec2 d = v[i + 1].pos - Vec2{10, 20};
    CHECK(std::sqrt(d.x * d.x + d.y * d.y) == doctest::Approx(12));
  }
}

TEST_CASE("outline is emitted first and larger, fill after") {
  const std::vector<DrawItem> items{{{Rect{{10, 10}}, kRed, kBlack}, {0, 0}, 0.0f}};
  const auto v = tessellate(items);
  REQUIRE(v.size() == 12);
  CHECK(v[0].color == kBlack);
  CHECK(v[6].color == kRed);
  CHECK(std::abs(v[0].pos.x) > std::abs(v[6].pos.x));
}

TEST_CASE("text placement honors align and centers vertically") {
  TextItem t{"ab", {100, 50}, 16, {}, Align::Left};
  auto p = place_text(t);
  CHECK(p.scale == 2);
  CHECK(p.top_left.x * p.scale == doctest::Approx(100));
  CHECK(p.top_left.y * p.scale == doctest::Approx(42));
  t.align = Align::Center;
  CHECK(place_text(t).top_left.x * 2 == doctest::Approx(84));
  t.align = Align::Right;
  CHECK(place_text(t).top_left.x * 2 == doctest::Approx(68));
}
