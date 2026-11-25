#include <doctest/doctest.h>

#include "platform/sdl_input.hpp"

using alpha::core::parse_input;
using namespace alpha::platform;

namespace {

SDL_Event key(SDL_Scancode code, bool down, bool repeat = false) {
  SDL_Event e{};
  e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
  e.key.scancode = code;
  e.key.down = down;
  e.key.repeat = repeat;
  return e;
}

SDL_Event mouse(Uint8 button, bool down) {
  SDL_Event e{};
  e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
  e.button.button = button;
  e.button.down = down;
  return e;
}

SDL_Event pad(Uint8 button, bool down) {
  SDL_Event e{};
  e.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
  e.gbutton.button = button;
  e.gbutton.down = down;
  return e;
}

bool has(const alpha::core::InputSet& s, const char* name) { return s.test(*parse_input(name)); }

}  // namespace

TEST_CASE("SDL devices map to core input names") {
  CHECK(from_scancode(SDL_SCANCODE_SPACE) == parse_input("Space"));
  CHECK(from_scancode(SDL_SCANCODE_R) == parse_input("R"));
  CHECK(from_scancode(SDL_SCANCODE_0) == parse_input("0"));
  CHECK(from_scancode(SDL_SCANCODE_1) == parse_input("1"));
  CHECK(from_scancode(SDL_SCANCODE_RIGHT) == parse_input("Right"));
  CHECK_FALSE(from_scancode(SDL_SCANCODE_F1));
  CHECK(from_mouse(SDL_BUTTON_LEFT) == parse_input("MouseLeft"));
  CHECK(from_gamepad(SDL_GAMEPAD_BUTTON_SOUTH) == parse_input("GamepadA"));
}

TEST_CASE("a tap between two steps still reaches the next step once") {
  InputState s;
  apply_event(s, key(SDL_SCANCODE_SPACE, true));
  apply_event(s, key(SDL_SCANCODE_SPACE, false));
  CHECK(has(consume(s), "Space"));
  CHECK_FALSE(has(consume(s), "Space"));
}

TEST_CASE("held inputs stay down across steps; repeats and unrelated events are ignored") {
  InputState s;
  apply_event(s, mouse(SDL_BUTTON_LEFT, true));
  apply_event(s, pad(SDL_GAMEPAD_BUTTON_SOUTH, true));
  apply_event(s, key(SDL_SCANCODE_A, true, true));
  SDL_Event quit{};
  quit.type = SDL_EVENT_QUIT;
  apply_event(s, quit);
  consume(s);
  const auto held = consume(s);
  CHECK(has(held, "MouseLeft"));
  CHECK(has(held, "GamepadA"));
  CHECK_FALSE(has(held, "A"));
  apply_event(s, mouse(SDL_BUTTON_LEFT, false));
  apply_event(s, pad(SDL_GAMEPAD_BUTTON_SOUTH, false));
  CHECK(consume(s).none());
}
