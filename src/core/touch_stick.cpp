#include "core/touch_stick.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "core/axes.hpp"

namespace alpha::core {

namespace {

// The direction turned to the nearest of `n` evenly spaced ones (0 = right).
Vec2 snapped(Vec2 v, int n) {
  const float len = std::hypot(v.x, v.y);
  if (n == 0 || len == 0.0f) return v;
  const float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(n);
  const float angle = std::round(std::atan2(v.y, v.x) / step) * step;
  const auto clean = [](float c) { return std::fabs(c) < 1e-6f ? 0.0f : c; };
  return {clean(std::cos(angle) * len), clean(std::sin(angle) * len)};
}

}  // namespace

Vec2 in_safe(const TouchScreen& screen, Vec2 fraction) {
  const Vec2 size = screen.safe.max - screen.safe.min;
  return {screen.safe.min.x + fraction.x * size.x, screen.safe.min.y + fraction.y * size.y};
}

Vec2 stick_value(Vec2 offset, const StickConfig& stick) {
  Vec2 v = offset * (1.0f / stick.radius);
  const float len = std::hypot(v.x, v.y);
  if (len > 1.0f) v = v * (1.0f / len);
  const auto dz = radial_deadzone(v.x, v.y, stick.deadzone);
  v = {dz[0], dz[1]};
  if (stick.axes == StickAxes::X) v.y = 0.0f;
  if (stick.axes == StickAxes::Y) v.x = 0.0f;
  return snapped(v, stick.snap);
}

Box stick_zone(const StickConfig& stick, const TouchScreen& screen) {
  return {in_safe(screen, stick.zone.min), in_safe(screen, stick.zone.max)};
}

Vec2 stick_anchor(const StickConfig& stick, const TouchScreen& screen) {
  const Vec2 center = (stick.zone.min + stick.zone.max) * 0.5f;
  return in_safe(screen, stick.anchor.value_or(center));
}

Vec2 stick_center(const StickConfig& stick, const TouchScreen& screen, Vec2 touch) {
  if (stick.mode == StickMode::Fixed) return stick_anchor(stick, screen);
  const auto fit = [r = stick.radius](float v, float lo, float hi) {
    return lo + r <= hi - r ? std::clamp(v, lo + r, hi - r) : (lo + hi) * 0.5f;
  };
  return {fit(touch.x, screen.safe.min.x, screen.safe.max.x), fit(touch.y, screen.safe.min.y, screen.safe.max.y)};
}

bool contains(const Box& box, Vec2 p) {
  return p.x >= box.min.x && p.x < box.max.x && p.y >= box.min.y && p.y < box.max.y;
}

}  // namespace alpha::core
