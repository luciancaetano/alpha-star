#include "core/touch_button.hpp"

#include <cmath>

namespace alpha::core {

namespace {

float distance(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

}  // namespace

bool button_hit(const ButtonConfig& button, const TouchScreen& screen, Vec2 p) {
  return distance(in_safe(screen, button.pos), p) <= button.radius + button.hit_slop;
}

bool buttons_overlap(const ButtonConfig& a, const ButtonConfig& b, const TouchScreen& screen) {
  const float reach = a.radius + a.hit_slop + b.radius + b.hit_slop;
  return distance(in_safe(screen, a.pos), in_safe(screen, b.pos)) < reach;
}

}  // namespace alpha::core
