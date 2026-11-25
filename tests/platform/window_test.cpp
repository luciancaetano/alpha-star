#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include "platform/window.hpp"

using alpha::core::Scaling;
using alpha::core::Vec2;
using alpha::core::WindowConfig;
using alpha::core::WindowState;

namespace {

SDL_Event key(SDL_Scancode code, SDL_Keymod mod, bool repeat = false) {
  SDL_Event e{};
  e.type = SDL_EVENT_KEY_DOWN;
  e.key.scancode = code;
  e.key.mod = mod;
  e.key.repeat = repeat;
  return e;
}

}  // namespace

TEST_CASE("Alt+Enter asks for the other fullscreen state unless the game opted out") {
  WindowState state;
  const WindowConfig on;
  CHECK(alpha::platform::window_event(key(SDL_SCANCODE_RETURN, SDL_KMOD_LALT), on, state));
  CHECK(state.request == true);
  state = {true, {}, {}};
  CHECK(alpha::platform::window_event(key(SDL_SCANCODE_RETURN, SDL_KMOD_RALT), on, state));
  CHECK(state.request == false);
  WindowState untouched;
  CHECK_FALSE(alpha::platform::window_event(key(SDL_SCANCODE_RETURN, SDL_KMOD_NONE), on, untouched));
  CHECK_FALSE(alpha::platform::window_event(key(SDL_SCANCODE_A, SDL_KMOD_LALT), on, untouched));
  CHECK_FALSE(alpha::platform::window_event(key(SDL_SCANCODE_RETURN, SDL_KMOD_LALT, true), on, untouched));
  WindowConfig off;
  off.alt_enter = false;
  CHECK_FALSE(alpha::platform::window_event(key(SDL_SCANCODE_RETURN, SDL_KMOD_LALT), off, untouched));
  CHECK_FALSE(untouched.request.has_value());
}

TEST_CASE("open_window sizes for the scaling mode; sync_window applies requests and reads back") {
  REQUIRE(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
  REQUIRE(SDL_InitSubSystem(SDL_INIT_VIDEO));
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  WindowConfig config;
  config.scaling = Scaling::Integer;
  config.resizable = false;
  config.borderless = true;
  config.always_on_top = true;
  REQUIRE(alpha::platform::open_window({"t", {160, 90}, config, true}, &window, &renderer));
  CHECK((SDL_GetWindowFlags(window) & SDL_WINDOW_RESIZABLE) == 0);
  CHECK((SDL_GetWindowFlags(window) & SDL_WINDOW_BORDERLESS) != 0);
  CHECK((SDL_GetWindowFlags(window) & SDL_WINDOW_ALWAYS_ON_TOP) != 0);
  WindowState state;
  alpha::platform::sync_window(window, state);
  CHECK(state.size == Vec2{320, 180});
  CHECK_FALSE(state.fullscreen);
  state.request = false;
  alpha::platform::sync_window(window, state);
  CHECK_FALSE(state.request.has_value());
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
