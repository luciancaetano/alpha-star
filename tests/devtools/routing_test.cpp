#include <doctest/doctest.h>

#include "devtools/routing.hpp"

using alpha::devtools::Capture;
using alpha::devtools::is_toggle;
using alpha::devtools::reaches_game;

namespace {

SDL_Event key(SDL_Scancode code, bool repeat = false) {
  SDL_Event e{};
  e.type = SDL_EVENT_KEY_DOWN;
  e.key.scancode = code;
  e.key.repeat = repeat;
  return e;
}

SDL_Event button(Uint8 b) {
  SDL_Event e{};
  e.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
  e.gbutton.button = b;
  return e;
}

SDL_Event of_type(SDL_EventType type) {
  SDL_Event e{};
  e.type = type;
  return e;
}

}  // namespace

TEST_CASE("F1 or gamepad Back+Start toggles the dev tools") {
  CHECK(is_toggle(key(SDL_SCANCODE_F1), false));
  CHECK_FALSE(is_toggle(key(SDL_SCANCODE_F1, true), false));
  CHECK_FALSE(is_toggle(key(SDL_SCANCODE_F2), false));
  CHECK(is_toggle(button(SDL_GAMEPAD_BUTTON_START), true));
  CHECK(is_toggle(button(SDL_GAMEPAD_BUTTON_BACK), true));
  CHECK_FALSE(is_toggle(button(SDL_GAMEPAD_BUTTON_START), false));
  CHECK_FALSE(is_toggle(button(SDL_GAMEPAD_BUTTON_SOUTH), true));
}

TEST_CASE("events ImGui captures do not reach the game") {
  const Capture none{false, false};
  const Capture mouse{true, false};
  const Capture keys{false, true};
  CHECK(reaches_game(key(SDL_SCANCODE_SPACE), none));
  CHECK(reaches_game(key(SDL_SCANCODE_SPACE), mouse));
  CHECK_FALSE(reaches_game(key(SDL_SCANCODE_SPACE), keys));
  CHECK_FALSE(reaches_game(of_type(SDL_EVENT_KEY_UP), keys));
  CHECK_FALSE(reaches_game(of_type(SDL_EVENT_MOUSE_BUTTON_DOWN), mouse));
  CHECK_FALSE(reaches_game(of_type(SDL_EVENT_MOUSE_BUTTON_UP), mouse));
  CHECK(reaches_game(of_type(SDL_EVENT_MOUSE_BUTTON_DOWN), keys));
  CHECK(reaches_game(button(SDL_GAMEPAD_BUTTON_SOUTH), Capture{true, true}));
}

TEST_CASE("the toggle key never reaches the game") {
  CHECK_FALSE(reaches_game(key(SDL_SCANCODE_F1), Capture{false, false}));
}
