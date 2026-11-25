#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

#include "core/image_filter.hpp"

using alpha::core::Image;
using alpha::core::kSharpMaxSide;
using alpha::core::kSharpScale;
using alpha::core::sharp_factor;
using alpha::core::upscaled;

TEST_CASE("sharp_factor: the whole scale that keeps the texture inside kSharpMaxSide") {
  CHECK(sharp_factor(32, 32) == kSharpScale);
  CHECK(sharp_factor(kSharpMaxSide / 2, 10) == 2);
  CHECK(sharp_factor(10, kSharpMaxSide / 3) == 3);
  CHECK(sharp_factor(kSharpMaxSide, 1) == 1);
  CHECK(sharp_factor(kSharpMaxSide + 1, 1) == 1);  // too big to grow: plain linear
}

TEST_CASE("upscaled: every texel becomes a k x k block") {
  const Image in{2, 1, {255, 0, 0, 255, 0, 0, 255, 128}};
  const Image out = upscaled(in, 3);
  REQUIRE(out.width == 6);
  REQUIRE(out.height == 3);
  for (int y = 0; y < 3; ++y) {
    for (int x = 0; x < 6; ++x) {
      const std::uint8_t* p = &out.rgba[static_cast<std::size_t>((y * 6 + x) * 4)];
      const std::vector<std::uint8_t> got(p, p + 4);
      CHECK(got == (x < 3 ? std::vector<std::uint8_t>{255, 0, 0, 255} : std::vector<std::uint8_t>{0, 0, 255, 128}));
    }
  }
  CHECK(upscaled(in, 1).rgba == in.rgba);
}
