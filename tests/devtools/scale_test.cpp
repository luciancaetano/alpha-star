#include <doctest/doctest.h>

#include "devtools/scale.hpp"

using alpha::devtools::tools_scale;

TEST_CASE("tools scale: a 1080p screen at 100% is the ImGui default size") {
  CHECK(tools_scale(1920, 1080, 1.0f) == doctest::Approx(1.0));
}

TEST_CASE("tools scale: grows with the screen's resolution (its shorter side)") {
  CHECK(tools_scale(3840, 2160, 1.0f) == doctest::Approx(2.0));
  CHECK(tools_scale(2160, 3840, 1.0f) == doctest::Approx(2.0));  // portrait: same screen
}

TEST_CASE("tools scale: follows the OS content scale (phones: dense screens)") {
  CHECK(tools_scale(1080, 2340, 2.625f) == doctest::Approx(2.625));
  CHECK(tools_scale(1920, 1080, 1.5f) == doctest::Approx(1.5));
}

TEST_CASE("tools scale: never below the default on small screens, nor gigantic") {
  CHECK(tools_scale(1280, 720, 1.0f) == doctest::Approx(1.0));
  CHECK(tools_scale(0, 0, 0.0f) == doctest::Approx(1.0));
  CHECK(tools_scale(10000, 10000, 9.0f) == doctest::Approx(4.0));
}
