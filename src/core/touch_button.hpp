#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "core/color.hpp"
#include "core/image.hpp"
#include "core/touch_stick.hpp"

namespace alpha::core {

// Images for a touch button; without `down` the pressed look is `up` darkened.
struct ButtonSkin {
  Picture up;
  std::optional<Picture> down;
  float size = 0.0f;  // diameter; 0 = radius * 2
  Color tint{255, 255, 255, 255};
};

// One config.touchButtons entry, validated (script/touch_config).
struct ButtonConfig {
  std::string name;  // PascalCase: "Jump" -> source TouchJump
  Vec2 pos{0.5f, 0.5f};  // center as a fraction of the safe area
  float radius = 40.0f;
  float hit_slop = 8.0f;
  std::string label;
  bool visible = true;
  float opacity = 0.6f;
  std::optional<ButtonSkin> skin;
  std::size_t player = 0;
  std::vector<std::string> scenes;  // where it exists; empty = every scene
};

// A finger landing at `p` grabs the button when within radius + hitSlop.
bool button_hit(const ButtonConfig& button, const TouchScreen& screen, Vec2 p);
// Two buttons whose touch areas overlap on `screen` (a config error: ambiguous).
bool buttons_overlap(const ButtonConfig& a, const ButtonConfig& b, const TouchScreen& screen);

}  // namespace alpha::core
