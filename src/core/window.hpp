#pragma once

#include <optional>
#include <string_view>

#include "core/vec2.hpp"

// Window presentation (pure): how the game's logical size maps to the window.
namespace alpha::core {

// letterbox: largest fit keeping the aspect, bars around; integer: largest
// whole-number scale (pixel art), bars around; stretch: fills the window,
// aspect lost; expand: fills the window keeping the aspect by showing more
// world (the logical size grows along one axis).
enum class Scaling { Letterbox, Integer, Stretch, Expand };

// "letterbox" | "integer" | "stretch" | "expand"; nullopt otherwise.
std::optional<Scaling> parse_scaling(std::string_view name);

// config.window, fixed at boot.
struct WindowConfig {
  bool fullscreen = false;
  Scaling scaling = Scaling::Letterbox;
  bool resizable = true;
  bool alt_enter = true;  // Alt+Enter toggles fullscreen
};

// What scripts see and ask for (ctx.window). The engine writes `fullscreen`
// and `size` (window pixels) every frame and applies `request` once.
struct WindowState {
  bool fullscreen = false;
  Vec2 size;
  std::optional<bool> request;
};

// Logical size for `expand`: the game size grown along one axis so its
// aspect matches the output's (never smaller than `game`).
Vec2 expanded_size(Vec2 game, Vec2 output);
// Initial window size: the game size times a scale that suits the mode
// (integer: a whole number, so pixels stay square from the first frame).
Vec2 initial_window_size(Vec2 game, Scaling scaling);

}  // namespace alpha::core
