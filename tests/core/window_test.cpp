#include <doctest/doctest.h>

#include "core/window.hpp"

using alpha::core::Aspect;
using alpha::core::Scale;
using alpha::core::Stretch;
using alpha::core::Vec2;

namespace {

alpha::core::Presentation shown(Vec2 design, Vec2 output, Aspect aspect, Scale scale = Scale::Fractional) {
  return alpha::core::present(design, output, Stretch{aspect, scale});
}

}  // namespace

TEST_CASE("aspect and scale names parse; anything else is rejected") {
  CHECK(alpha::core::parse_aspect("keep") == Aspect::Keep);
  CHECK(alpha::core::parse_aspect("keep_width") == Aspect::KeepWidth);
  CHECK(alpha::core::parse_aspect("keep_height") == Aspect::KeepHeight);
  CHECK(alpha::core::parse_aspect("expand") == Aspect::Expand);
  CHECK_FALSE(alpha::core::parse_aspect("letterbox").has_value());
  CHECK(alpha::core::parse_scale("fractional") == Scale::Fractional);
  CHECK(alpha::core::parse_scale("integer") == Scale::Integer);
  CHECK_FALSE(alpha::core::parse_scale("pixel").has_value());
  CHECK(Stretch{}.aspect == Aspect::Expand);  // games fill the screen unless they ask for bars
}

TEST_CASE("keep: the design size, letterboxed and centered") {
  const auto p = shown({320, 180}, {1920, 1200}, Aspect::Keep);
  CHECK(p.logical == Vec2{320, 180});
  CHECK(p.scale == doctest::Approx(6));
  CHECK(p.offset == Vec2{0, 60});
}

TEST_CASE("expand: more world on the longer axis, no bars (landscape and portrait)") {
  const auto wide = shown({320, 180}, {2340, 1080}, Aspect::Expand);
  CHECK(wide.logical == Vec2{390, 180});
  CHECK(wide.scale == doctest::Approx(6));
  CHECK(wide.offset == Vec2{0, 0});
  const auto tall = shown({288, 512}, {1080, 2340}, Aspect::Expand);
  CHECK(tall.logical == Vec2{288, 624});
  CHECK(tall.scale == doctest::Approx(3.75));
  CHECK(tall.offset.x == doctest::Approx(0));
  CHECK(tall.offset.y == doctest::Approx(0));
  CHECK(shown({320, 180}, {1920, 1080}, Aspect::Expand).logical == Vec2{320, 180});
}

TEST_CASE("keep_width reveals height only; keep_height reveals width only; the other axis gets bars") {
  CHECK(shown({288, 512}, {1080, 2340}, Aspect::KeepWidth).logical == Vec2{288, 624});
  const auto wide = shown({288, 512}, {2340, 1080}, Aspect::KeepWidth);
  CHECK(wide.logical == Vec2{288, 512});
  CHECK(wide.offset.x == doctest::Approx((2340 - 288 * 2.109375) / 2));
  CHECK(shown({320, 180}, {2340, 1080}, Aspect::KeepHeight).logical == Vec2{390, 180});
  CHECK(shown({320, 180}, {1080, 2340}, Aspect::KeepHeight).logical == Vec2{320, 180});
}

TEST_CASE("integer: whole-number scale; the revealed axes fill the output up to under one scale step") {
  const auto keep = shown({320, 180}, {1000, 700}, Aspect::Keep, Scale::Integer);
  CHECK(keep.logical == Vec2{320, 180});
  CHECK(keep.scale == doctest::Approx(3));
  CHECK(keep.offset == Vec2{20, 80});
  const auto fill = shown({320, 180}, {1000, 700}, Aspect::Expand, Scale::Integer);
  CHECK(fill.logical == Vec2{333, 233});
  CHECK(fill.scale == doctest::Approx(3));
  CHECK(fill.offset == Vec2{0.5f, 0.5f});
  CHECK(shown({320, 180}, {200, 100}, Aspect::Keep, Scale::Integer).scale == doctest::Approx(1));  // never below 1
}

TEST_CASE("a phone's display size turns to the game's orientation") {
  CHECK(alpha::core::oriented({1080, 2340}, {320, 180}) == Vec2{2340, 1080});
  CHECK(alpha::core::oriented({1080, 2340}, {288, 512}) == Vec2{1080, 2340});
  CHECK(alpha::core::oriented({2340, 1080}, {320, 180}) == Vec2{2340, 1080});
}

TEST_CASE("only phones boot laid out for their display") {
  CHECK(alpha::core::phone_display(alpha::core::Os::android, {1080, 2340}) == Vec2{1080, 2340});
  CHECK_FALSE(alpha::core::phone_display(alpha::core::Os::windows, {1920, 1080}).has_value());
  CHECK_FALSE(alpha::core::phone_display(alpha::core::Os::ios, {0, 0}).has_value());
}

TEST_CASE("odd and empty outputs stay sane") {
  const auto odd = shown({320, 180}, {1001, 999}, Aspect::Expand);
  CHECK(odd.logical.x == 320);
  CHECK(odd.logical.y == 319);  // round(999 / (1001 / 320))
  const auto none = shown({320, 180}, {0, 0}, Aspect::Expand);
  CHECK(none.logical == Vec2{320, 180});
  CHECK(none.scale == doctest::Approx(1));
}

TEST_CASE("the first window is 1.5x, or a whole 2x for integer scale, when the screen is unknown") {
  CHECK(alpha::core::initial_window_size({320, 180}, Scale::Fractional, {}) == Vec2{480, 270});
  CHECK(alpha::core::initial_window_size({320, 180}, Scale::Integer, {}) == Vec2{640, 360});
}

TEST_CASE("the first window fits the screen's usable area") {
  using alpha::core::initial_window_size;
  CHECK(initial_window_size({384, 216}, Scale::Integer, {1920, 1080}) == Vec2{768, 432});
  CHECK(initial_window_size({1024, 768}, Scale::Integer, {1920, 1080}) == Vec2{1024, 768});
  const Vec2 frac = initial_window_size({1024, 768}, Scale::Fractional, {1280, 720});
  CHECK(frac.x <= 1280);
  CHECK(frac.y <= 720);
  CHECK(frac.y == doctest::Approx(648));                                                   // 90% of the height
  CHECK(initial_window_size({4000, 3000}, Scale::Integer, {1920, 1080}) == Vec2{4000, 3000});  // never below 1x
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
