#include <doctest/doctest.h>

#include "core/window.hpp"

using alpha::core::Scaling;
using alpha::core::Vec2;

TEST_CASE("scaling names parse; anything else is rejected") {
  CHECK(alpha::core::parse_scaling("letterbox") == Scaling::Letterbox);
  CHECK(alpha::core::parse_scaling("integer") == Scaling::Integer);
  CHECK(alpha::core::parse_scaling("stretch") == Scaling::Stretch);
  CHECK(alpha::core::parse_scaling("expand") == Scaling::Expand);
  CHECK(alpha::core::parse_scaling("crop") == Scaling::Crop);
  CHECK_FALSE(alpha::core::parse_scaling("fit").has_value());
}

TEST_CASE("expand grows the logical size along one axis to the output's aspect") {
  CHECK(alpha::core::expanded_size({320, 180}, {1920, 1080}) == Vec2{320, 180});
  CHECK(alpha::core::expanded_size({320, 180}, {1920, 1200}) == Vec2{320, 200});
  CHECK(alpha::core::expanded_size({320, 180}, {2560, 1080}) == Vec2{(2560.0f / 1080.0f) * 180.0f, 180});
  CHECK(alpha::core::expanded_size({320, 180}, {0, 0}) == Vec2{320, 180});
}

TEST_CASE("the first window is 1.5x, or a whole 2x for integer scaling") {
  CHECK(alpha::core::initial_window_size({320, 180}, Scaling::Letterbox) == Vec2{480, 270});
  CHECK(alpha::core::initial_window_size({320, 180}, Scaling::Integer) == Vec2{640, 360});
}

TEST_CASE("phones always start fullscreen; desktop follows config.window.fullscreen") {
  alpha::core::WindowConfig c;
  CHECK(alpha::core::starts_fullscreen(c, alpha::core::Os::android));
  CHECK(alpha::core::starts_fullscreen(c, alpha::core::Os::ios));
  CHECK_FALSE(alpha::core::starts_fullscreen(c, alpha::core::Os::windows));
  c.fullscreen = true;
  CHECK(alpha::core::starts_fullscreen(c, alpha::core::Os::linux_os));
}

TEST_CASE("safe insets are the unsafe output edges in logical pixels, never negative") {
  // 320x180 shown at 3x with 60 px letterbox bars left and right (output 1080 wide, 540 tall).
  const alpha::core::PixelGrid grid{{3.0f, 3.0f}, {60.0f, 0.0f}};
  const auto in = alpha::core::safe_insets(grid, {320, 180}, {90, 0}, {1080, 510});
  CHECK(in.left == doctest::Approx(10));    // cutout reaches 30 px past the bar
  CHECK(in.top == doctest::Approx(0));
  CHECK(in.right == doctest::Approx(0));    // safe edge lies in the bar: nothing hidden
  CHECK(in.bottom == doctest::Approx(10));  // 30 px of rounded corners / gesture area
}
