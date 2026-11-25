#include "platform/window.hpp"

namespace alpha::platform {

bool open_window(const WindowSetup& setup, SDL_Window** window, SDL_Renderer** renderer) {
  const core::WindowConfig& c = setup.config;
  SDL_WindowFlags flags = 0;
  if (c.resizable) flags |= SDL_WINDOW_RESIZABLE;
  if (c.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
  if (setup.hidden) flags |= SDL_WINDOW_HIDDEN;
  const core::Vec2 size = core::initial_window_size(setup.game, c.scaling);
  return SDL_CreateWindowAndRenderer(setup.title.c_str(), static_cast<int>(size.x), static_cast<int>(size.y), flags,
                                     window, renderer);
}

bool window_event(const SDL_Event& event, const core::WindowConfig& config, core::WindowState& state) {
  if (!config.alt_enter || event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) return false;
  if (event.key.scancode != SDL_SCANCODE_RETURN || (event.key.mod & SDL_KMOD_ALT) == 0) return false;
  state.request = !state.fullscreen;
  return true;
}

void sync_window(SDL_Window* window, core::WindowState& state) {
  if (state.request) {
    SDL_SetWindowFullscreen(window, *state.request);
    SDL_SyncWindow(window);
    state.request.reset();
  }
  state.fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
  int w = 0;
  int h = 0;
  SDL_GetWindowSizeInPixels(window, &w, &h);
  state.size = {static_cast<float>(w), static_cast<float>(h)};
}

}  // namespace alpha::platform
