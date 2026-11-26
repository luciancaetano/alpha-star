#include "core/window.hpp"

#include <algorithm>
#include <cmath>

namespace alpha::core {

std::optional<Aspect> parse_aspect(std::string_view name) {
  if (name == "keep") return Aspect::Keep;
  if (name == "keep_width") return Aspect::KeepWidth;
  if (name == "keep_height") return Aspect::KeepHeight;
  if (name == "expand") return Aspect::Expand;
  return std::nullopt;
}

std::optional<Scale> parse_scale(std::string_view name) {
  if (name == "fractional") return Scale::Fractional;
  if (name == "integer") return Scale::Integer;
  return std::nullopt;
}

namespace {

bool reveals_x(Aspect a) { return a == Aspect::Expand || a == Aspect::KeepHeight; }
bool reveals_y(Aspect a) { return a == Aspect::Expand || a == Aspect::KeepWidth; }

// The design size grown along a revealed axis to fill `room` game pixels (whole pixels).
float grown(float design, float room, bool reveals, Scale scale) {
  if (!reveals) return design;
  return std::max(design, scale == Scale::Integer ? std::floor(room) : std::round(room));
}

}  // namespace

// The scale that fits the design size (integer: floored, at least 1), then
// the revealed axes fill the rest; the final scale is the one that fits the
// grown size, as SDL's logical presentation computes it.
Presentation present(Vec2 design, Vec2 output, Stretch stretch) {
  if (output.x <= 0 || output.y <= 0) return {design, 1.0f, {}};
  const bool integer = stretch.scale == Scale::Integer;
  float fit = std::min(output.x / design.x, output.y / design.y);
  if (integer) fit = std::max(1.0f, std::floor(fit));
  const Vec2 logical{grown(design.x, output.x / fit, reveals_x(stretch.aspect), stretch.scale),
                     grown(design.y, output.y / fit, reveals_y(stretch.aspect), stretch.scale)};
  float scale = std::min(output.x / logical.x, output.y / logical.y);
  if (integer) scale = std::max(1.0f, std::floor(scale));
  return {logical, scale, (output - logical * scale) * 0.5f};
}

Vec2 oriented(Vec2 screen, Vec2 design) {
  return (screen.x > screen.y) == (design.x > design.y) ? screen : Vec2{screen.y, screen.x};
}

std::optional<Vec2> phone_display(Os os, Vec2 display) {
  if (!is_mobile(os) || display.x <= 0 || display.y <= 0) return std::nullopt;
  return display;
}

Vec2 initial_window_size(Vec2 game, Scale scale, Vec2 usable) {
  const bool integer = scale == Scale::Integer;
  if (usable.x <= 0 || usable.y <= 0) return game * (integer ? 2.0f : 1.5f);
  const float fit = std::min(usable.x / game.x, usable.y / game.y) * 0.9f;  // room for the title bar
  return game * (integer ? std::clamp(std::floor(fit), 1.0f, 2.0f) : std::min(1.5f, fit));
}

bool starts_fullscreen(const WindowConfig& config, Os os) { return config.fullscreen || is_mobile(os); }

Insets safe_insets(const PixelGrid& grid, Vec2 view, Vec2 safe_min, Vec2 safe_max) {
  const auto logical = [&grid](Vec2 p) {
    return Vec2{(p.x - grid.offset.x) / grid.scale.x, (p.y - grid.offset.y) / grid.scale.y};
  };
  const Vec2 lo = logical(safe_min);
  const Vec2 hi = logical(safe_max);
  return {std::max(0.0f, lo.x), std::max(0.0f, lo.y), std::max(0.0f, view.x - hi.x), std::max(0.0f, view.y - hi.y)};
}

}  // namespace alpha::core
