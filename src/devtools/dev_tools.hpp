#pragma once

#include <SDL3/SDL.h>

#include "devtools/panels.hpp"
#include "devtools/snapshot.hpp"
#include "devtools/time_control.hpp"
#include "render/renderer.hpp"

namespace alpha::devtools {

// Built-in engine dev tools (Dear ImGui), toggled with F12 or gamepad
// Back+Start. Only in ALPHA_DEVTOOLS builds; never visible to game scripts.
// Owns the ImGui context for one window/renderer pair.
class DevTools {
 public:
  DevTools(SDL_Window* window, SDL_Renderer* sdl);
  ~DevTools();
  DevTools(const DevTools&) = delete;
  DevTools& operator=(const DevTools&) = delete;

  bool open() const { return open_; }
  void toggle() { open_ = !open_; }
  // Feeds one event to ImGui; returns whether the game may also see it.
  bool handle(const SDL_Event& event);
  // Steps the simulation runs this frame (time controls apply even when closed).
  int steps(int wanted) { return time_.steps(wanted); }
  bool paused() const { return time_.paused(); }
  // Adds dev overlays drawn with the game (physics view) to its frame.
  void decorate(render::Frame& frame, const ecs::World* world) const;
  // Draws the open tools over the rendered game frame, before present.
  void draw(script::Host& host, FrameStats stats);

  Panels& panels() { return panels_; }
  TimeControl& time() { return time_; }

 private:
  bool toggle_requested(const SDL_Event& event) const;

  SDL_Renderer* sdl_;
  bool open_ = false;
  Panels panels_;
  TimeControl time_;
};

}  // namespace alpha::devtools
