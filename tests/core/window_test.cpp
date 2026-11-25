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
