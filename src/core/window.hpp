#pragma once

#include <optional>
#include <string_view>

#include "core/os.hpp"
#include "core/pixel_grid.hpp"
#include "core/vec2.hpp"

// Window presentation (pure): how the game's logical size maps to the window.
namespace alpha::core {

// Godot-style stretch: the game is designed at config.size; `aspect` says
// what happens when the output's aspect differs. keep: letterbox bars;
// keep_width: the width stays, a taller output shows more world (wider: bars);
// keep_height: the height stays, a wider output shows more world (taller:
// bars); expand: more world on whichever axis is longer (never bars).
enum class Aspect { Keep, KeepWidth, KeepHeight, Expand };
// fractional: any scale; integer: whole multiples only (crisp pixel art).
enum class Scale { Fractional, Integer };

struct Stretch {
  Aspect aspect = Aspect::Expand;
  Scale scale = Scale::Fractional;
};

// "keep" | "keep_width" | "keep_height" | "expand"; nullopt otherwise.
std::optional<Aspect> parse_aspect(std::string_view name);
// "fractional" | "integer"; nullopt otherwise.
std::optional<Scale> parse_scale(std::string_view name);

// How the game is shown in an output of `output` pixels: `logical` game
// pixels (config.size grown along the revealed axes, whole pixels), each
// `scale` output pixels wide, centered (`offset` = the bars, if any).
struct Presentation {
  Vec2 logical;
  float scale = 1.0f;
  Vec2 offset;
};
Presentation present(Vec2 design, Vec2 output, Stretch stretch);
// `screen` turned to the orientation of `design` (phones rotate to the game's
// orientation, so a portrait display size serves a landscape game swapped).
Vec2 oriented(Vec2 screen, Vec2 design);
// The screen a game boots on: a phone's `display` (its window fills it);
// nullopt on desktop or for an unknown (empty) display.
std::optional<Vec2> phone_display(Os os, Vec2 display);

// config.window, fixed at boot.
struct WindowConfig {
  bool fullscreen = false;
  Stretch stretch;
  bool resizable = true;
  bool alt_enter = true;  // Alt+Enter toggles fullscreen
  bool borderless = false;     // no title bar or frame
  bool always_on_top = false;
  bool maximized = false;      // starts maximized (resizable windows)
  bool vsync = true;           // wait for the display refresh (false: as fast as possible)
};

// How far system UI (display cutouts, rounded corners) reaches into the game
// view on each side, in logical pixels (ctx.safeArea). Zero on desktop.
struct Insets {
  float left = 0;
  float top = 0;
  float right = 0;
  float bottom = 0;
};

// What scripts see and ask for (ctx.window). The engine writes `fullscreen`
// and `size` (window pixels) every frame and applies `request` once.
struct WindowState {
  bool fullscreen = false;
  Vec2 size;
  std::optional<bool> request;
  Insets safe;  // written every frame with size
};

// Phones always open fullscreen (immersive: no status or navigation bar);
// desktop follows config.window.fullscreen.
bool starts_fullscreen(const WindowConfig& config, Os os);
// The insets of a game `view` drawn through `grid` (output pixels) when only
// the output rect [safe_min, safe_max) is safe; never negative.
Insets safe_insets(const PixelGrid& grid, Vec2 view, Vec2 safe_min, Vec2 safe_max);

// Initial window size: the game size times a scale that suits the mode
// (integer: a whole number, so pixels stay square from the first frame).
Vec2 initial_window_size(Vec2 game, Scale scale);

}  // namespace alpha::core
