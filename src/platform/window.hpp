#pragma once

#include <SDL3/SDL.h>

#include <string>

#include "core/window.hpp"

namespace alpha::platform {

struct WindowSetup {
  std::string title;
  core::Vec2 game;  // config.size
  core::WindowConfig config;
  bool hidden = false;  // agent runs
};

// Creates the window (sized for the scaling mode; fullscreen and resizable
// from config.window) and its renderer. False on failure (SDL_GetError).
bool open_window(const WindowSetup& setup, SDL_Window** window, SDL_Renderer** renderer);
// Alt+Enter (unless config.window.altEnter is false) asks for the other
// fullscreen state. Returns true when it consumed the event.
bool window_event(const SDL_Event& event, const core::WindowConfig& config, core::WindowState& state);
// Applies a pending fullscreen request, then records the real state
// (fullscreen flag, window size in pixels) for ctx.window.
void sync_window(SDL_Window* window, core::WindowState& state);

}  // namespace alpha::platform
