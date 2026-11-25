#include <doctest/doctest.h>

#include <cmath>

#include "core/camera.hpp"

using namespace alpha::core;

namespace {

bool near(Vec2 a, Vec2 b, float eps = 1e-3f) {
  return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps;
}

constexpr float kDt = 1.0f / 60.0f;

}  // namespace

TEST_CASE("identity camera maps world pixels to the same screen pixels") {
  const Camera c = make_camera({320, 180});
  CHECK(c.pos == Vec2{160, 90});
  const View v = view_of(c);
  CHECK(near(to_screen(v, {10, 20}), {10, 20}));
  CHECK(near(to_world(v, {300, 5}), {300, 5}));
}

TEST_CASE("zoom and position scale around the screen center; to_world inverts to_screen") {
  const View v{{1000, 500}, 2.0f, 30.0f, {320, 180}};
  CHECK(near(to_screen(v, {1000, 500}), {160, 90}));
  const View flat{{1000, 500}, 2.0f, 0.0f, {320, 180}};
  CHECK(near(to_screen(flat, {1010, 500}), {180, 90}));
  for (const Vec2 p : {Vec2{0, 0}, Vec2{1234, -55}, Vec2{999, 501}}) {
    CHECK(near(to_world(v, to_screen(v, p)), p));
  }
}

TEST_CASE("rotation turns the world the other way on screen") {
  const View v{{0, 0}, 1.0f, 90.0f, {100, 100}};
  // A point to the right of the center appears above it when the camera turns 90 degrees.
  CHECK(near(to_screen(v, {10, 0}), {50, 40}));
}

TEST_CASE("visible area covers the screen in world coordinates") {
  const Box b = visible_area({{100, 100}, 2.0f, 0.0f, {200, 100}});
  CHECK(near(b.min, {50, 75}));
  CHECK(near(b.max, {150, 125}));
  const Box turned = visible_area({{0, 0}, 1.0f, 45.0f, {100, 100}});
  CHECK(turned.max.x == doctest::Approx(70.71f).epsilon(0.01));
  CHECK(overlaps(b, {{140, 120}, {160, 130}}));
  CHECK_FALSE(overlaps(b, {{151, 0}, {160, 300}}));
}

TEST_CASE("follow with lerp 1 locks onto the target; smaller lerp closes part of the gap") {
  Camera c = make_camera({100, 100});
  c.follow = Follow{1.0f, {}};
  step_camera(c, Vec2{300, 200}, kDt);
  CHECK(near(c.pos, {300, 200}));
  c.follow = Follow{0.5f, {}};
  step_camera(c, Vec2{400, 200}, kDt);
  CHECK(near(c.pos, {350, 200}));
}

TEST_CASE("the target moves inside the deadzone without moving the camera") {
  Camera c = make_camera({100, 100});
  c.follow = Follow{1.0f, {40, 20}};
  step_camera(c, Vec2{65, 55}, kDt);
  CHECK(near(c.pos, {50, 50}));
  step_camera(c, Vec2{80, 50}, kDt);  // 10 px past the deadzone's right edge (70)
  CHECK(near(c.pos, {60, 50}));
}

TEST_CASE("bounds keep the visible area inside, centered when the bounds are smaller") {
  Camera c = make_camera({100, 100});
  c.bounds = Box{{0, 0}, {400, 300}};
  c.pos = {10, 290};
  step_camera(c, std::nullopt, kDt);
  CHECK(near(c.pos, {50, 250}));
  c.zoom = 0.25f;  // sees 400 x 400 > bounds height
  step_camera(c, std::nullopt, kDt);
  CHECK(near(c.pos, {200, 150}));
}

TEST_CASE("shake is deterministic, decays, and never moves pos") {
  Camera a = make_camera({100, 100});
  Camera b = make_camera({100, 100});
  start_shake(a, 8.0f, 0.5f);
  start_shake(b, 8.0f, 0.5f);
  bool moved = false;
  for (int i = 0; i < 20; ++i) {
    step_camera(a, std::nullopt, kDt);
    step_camera(b, std::nullopt, kDt);
    CHECK(a.shake_offset == b.shake_offset);
    CHECK(std::abs(a.shake_offset.x) <= 8.0f);
    moved = moved || a.shake_offset.x != 0.0f;
  }
  CHECK(moved);
  CHECK(a.pos == Vec2{50, 50});
  for (int i = 0; i < 20; ++i) step_camera(a, std::nullopt, kDt);
  CHECK(a.shake_offset == Vec2{0, 0});
  CHECK(view_of(a).center == Vec2{50, 50});
}

TEST_CASE("blend interpolates center, zoom and rotation") {
  const View a{{0, 0}, 1.0f, 0.0f, {10, 10}};
  const View b{{10, 20}, 3.0f, 90.0f, {10, 10}};
  const View m = blend(a, b, 0.5f);
  CHECK(near(m.center, {5, 10}));
  CHECK(m.zoom == doctest::Approx(2.0f));
  CHECK(m.rotation == doctest::Approx(45.0f));
}
