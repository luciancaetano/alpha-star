#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/camera.hpp"
#include "core/color.hpp"
#include "core/image.hpp"
#include "core/vec2.hpp"

namespace alpha::core {

// Where the game's touch controls live: the safe area (ctx.safeArea) inside
// the view, both in game pixels.
struct TouchScreen {
  Box safe;
  Vec2 view;
};
// A point given as a fraction (0..1) of the safe area, in game pixels.
Vec2 in_safe(const TouchScreen& screen, Vec2 fraction);

// Images drawn instead of the default look (config.*.skin), already resolved.
// A missing `pressed` picture shows the resting one; size 0 = the default.
struct StickSkin {
  Picture base;
  Picture knob;
  std::optional<Picture> base_pressed;
  std::optional<Picture> knob_pressed;
  float base_size = 0.0f;
  float knob_size = 0.0f;
  Color tint{255, 255, 255, 255};
};

enum class StickMode { Floating, Fixed };
enum class StickAxes { Both, X, Y };

// One config.touchSticks entry, validated (script/touch_config).
struct StickConfig {
  std::string name;  // source name part, PascalCase: "Move" -> TouchMoveX
  Box zone{{0.0f, 0.0f}, {0.5f, 1.0f}};  // fractions of the safe area
  StickMode mode = StickMode::Floating;
  std::optional<Vec2> anchor;  // fixed: center as a fraction; default the zone's center
  float radius = 64.0f;
  float deadzone = 0.15f;
  StickAxes axes = StickAxes::Both;
  int snap = 0;  // 0 (analog), 4 or 8 directions
  bool always_visible = false;
  float opacity = 0.6f;
  std::optional<StickSkin> skin;
  std::size_t player = 0;
  std::vector<std::string> scenes;  // where it exists; empty = every scene
};

// The stick's value for a finger `offset` px from its center: length capped
// at 1, radial deadzone remapped to 0..1, then `axes` and `snap`.
Vec2 stick_value(Vec2 offset, const StickConfig& stick);

// Where a stick's base sits when a finger lands at `touch` (floating: there,
// moved to fit inside the safe area; fixed: its anchor).
Vec2 stick_center(const StickConfig& stick, const TouchScreen& screen, Vec2 touch);
// The resting base of a fixed stick (also drawn while untouched).
Vec2 stick_anchor(const StickConfig& stick, const TouchScreen& screen);
// The zone in game pixels.
Box stick_zone(const StickConfig& stick, const TouchScreen& screen);
bool contains(const Box& box, Vec2 p);

}  // namespace alpha::core
