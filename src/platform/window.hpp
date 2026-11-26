#pragma once

#include <SDL3/SDL.h>

#include <string>

#include "core/pixel_grid.hpp"
#include "core/window.hpp"

namespace alpha::platform {

// How the renderer's logical presentation maps game pixels to output pixels
// (identity without one).
core::PixelGrid output_grid(SDL_Renderer* renderer);

struct WindowSetup {
  std::string title;
  core::Vec2 game;  // config.size
  core::WindowConfig config;
  bool hidden = false;  // agent runs
  bool gpu = false;     // assets.shaders: SDL's GPU renderer on Vulkan (custom shaders), else SDL's default
};

// SDL's GPU renderer on the Vulkan driver, told we bring SPIR-V shaders
// (custom shaders run only there); nullptr when Vulkan is unavailable (SDL_GetError).
SDL_Renderer* vulkan_renderer(SDL_Window* window);
// Creates the window (sized for the scale mode; fullscreen, resizable,
// borderless, alwaysOnTop and maximized from config.window) and its renderer. False on failure (SDL_GetError).
bool open_window(const WindowSetup& setup, SDL_Window** window, SDL_Renderer** renderer);
// Alt+Enter (unless config.window.altEnter is false) asks for the other
// fullscreen state. Returns true when it consumed the event.
bool window_event(const SDL_Event& event, const core::WindowConfig& config, core::WindowState& state);
// Applies a pending fullscreen request, then records the real state
// (fullscreen flag, window size in pixels) for ctx.window.
void sync_window(SDL_Window* window, core::WindowState& state);

}  // namespace alpha::platform
