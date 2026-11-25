#include <doctest/doctest.h>

#include "core/pixel_grid.hpp"

using alpha::core::PixelGrid;
using alpha::core::Vec2;

TEST_CASE("pixel grid: logical size shown in an output rect gives scale and offset") {
  const PixelGrid g = alpha::core::pixel_grid({0, 128}, {432, 768}, {288, 512});
  CHECK(g.scale == Vec2{1.5f, 1.5f});
  CHECK(g.offset == Vec2{0, 128});
  CHECK(alpha::core::pixel_grid({0, 0}, {0, 0}, {288, 512}).scale == Vec2{1, 1});  // nothing shown yet
}

TEST_CASE("snap: a logical point moves to the nearest whole output pixel") {
  const PixelGrid g{{1.5f, 1.5f}, {0.5f, 0}};
  const Vec2 s = alpha::core::snap(g, {1.0f, 1.0f});  // output (2.0, 1.5) -> (2, 2)
  CHECK(s.x * 1.5f + 0.5f == doctest::Approx(2.0f));
  CHECK(s.y * 1.5f == doctest::Approx(2.0f));
  const PixelGrid one{};
  CHECK(alpha::core::snap(one, {3.4f, 3.6f}) == Vec2{3, 4});
}
