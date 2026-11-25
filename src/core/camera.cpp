#include "core/camera.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace alpha::core {

namespace {


// Moves `pos` only as far as needed to keep `target` within `half` of it.
float into_deadzone(float pos, float target, float half) {
  if (target > pos + half) return target - half;
  if (target < pos - half) return target + half;
  return pos;
}

// The center that keeps a `half`-wide view inside [lo, hi] (centered if it cannot).
float clamped(float pos, float half, float lo, float hi) {
  if (hi - lo <= 2.0f * half) return (lo + hi) * 0.5f;
  return std::clamp(pos, lo + half, hi - half);
}

// xorshift32 in [-1, 1].
float noise(std::uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return static_cast<float>(state) / 2147483647.5f - 1.0f;
}

void follow(Camera& c, Vec2 target) {
  const Vec2 half = c.follow->deadzone * 0.5f;
  const Vec2 wanted{into_deadzone(c.pos.x, target.x, half.x), into_deadzone(c.pos.y, target.y, half.y)};
  c.pos = c.pos + (wanted - c.pos) * c.follow->lerp;
}

void clamp_to_bounds(Camera& c) {
  const Vec2 half = c.size * (0.5f / c.zoom);
  c.pos = {clamped(c.pos.x, half.x, c.bounds->min.x, c.bounds->max.x),
           clamped(c.pos.y, half.y, c.bounds->min.y, c.bounds->max.y)};
}

void advance_shake(Camera& c, float dt) {
  c.shake_left = std::max(0.0f, c.shake_left - dt);
  if (c.shake_left <= 0.0f) {
    c.shake_offset = {};
    return;
  }
  const float strength = c.shake_intensity * c.shake_left / c.shake_total;
  const float x = noise(c.noise);
  const float y = noise(c.noise);
  c.shake_offset = Vec2{x, y} * strength;
}

}  // namespace

Vec2 rotated(Vec2 v, float degrees) {
  constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;
  const float c = std::cos(degrees * kDegToRad);
  const float s = std::sin(degrees * kDegToRad);
  return {v.x * c - v.y * s, v.x * s + v.y * c};
}

Vec2 to_screen(const View& view, Vec2 world) {
  return rotated(world - view.center, -view.rotation) * view.zoom + view.size * 0.5f;
}

Vec2 to_world(const View& view, Vec2 screen) {
  return rotated((screen - view.size * 0.5f) * (1.0f / view.zoom), view.rotation) + view.center;
}

Box visible_area(const View& view) {
  const std::array<Vec2, 4> corners{Vec2{0, 0}, Vec2{view.size.x, 0}, view.size, Vec2{0, view.size.y}};
  Box box{to_world(view, corners[0]), to_world(view, corners[0])};
  for (const Vec2 c : corners) {
    const Vec2 w = to_world(view, c);
    box.min = {std::min(box.min.x, w.x), std::min(box.min.y, w.y)};
    box.max = {std::max(box.max.x, w.x), std::max(box.max.y, w.y)};
  }
  return box;
}

View blend(const View& a, const View& b, float t) {
  const float turn = std::remainder(b.rotation - a.rotation, 360.0f);
  return {a.center + (b.center - a.center) * t, a.zoom + (b.zoom - a.zoom) * t, a.rotation + turn * t, b.size};
}

bool overlaps(const Box& a, const Box& b) {
  return a.min.x <= b.max.x && b.min.x <= a.max.x && a.min.y <= b.max.y && b.min.y <= a.max.y;
}

Camera make_camera(Vec2 size) {
  Camera c;
  c.size = size;
  c.pos = size * 0.5f;
  return c;
}

void start_shake(Camera& camera, float intensity, float seconds) {
  camera.shake_intensity = intensity;
  camera.shake_total = seconds;
  camera.shake_left = seconds;
}

void step_camera(Camera& camera, std::optional<Vec2> target, float dt) {
  if (camera.follow && target) follow(camera, *target);
  if (camera.bounds) clamp_to_bounds(camera);
  advance_shake(camera, dt);
}

View view_of(const Camera& camera) {
  return {camera.pos + camera.shake_offset, camera.zoom, camera.rotation, camera.size};
}

View parallax_view(const View& view, float parallax) {
  if (parallax == 1.0f) return view;
  const Vec2 home = view.size * 0.5f;
  return {home + (view.center - home) * parallax, 1.0f + (view.zoom - 1.0f) * parallax, view.rotation * parallax,
          view.size};
}

}  // namespace alpha::core
