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

SDL_Event pad(Uint8 button, bool down, SDL_JoystickID which = 7) {
  SDL_Event e{};
  e.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
  e.gbutton.which = which;
  e.gbutton.button = button;
  e.gbutton.down = down;
  return e;
}

SDL_Event axis(Uint8 which_axis, Sint16 value, SDL_JoystickID which = 7) {
  SDL_Event e{};
  e.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
  e.gaxis.which = which;
  e.gaxis.axis = which_axis;
  e.gaxis.value = value;
  return e;
}

bool has(const alpha::core::InputSet& s, const char* name) { return s.test(*parse_input(name)); }

// Player 1's held inputs for this step.
alpha::core::InputSet held1(InputState& s, const alpha::core::PadSlots& slots) {
  return consume(s, slots).players[0].held;
}

alpha::core::PadSlots one_pad() {
  alpha::core::PadSlots slots;
  slots.connect(7, "pad");
  return slots;
}

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
  CHECK(from_gamepad(SDL_GAMEPAD_BUTTON_LEFT_STICK) == parse_input("GamepadLS"));
}

TEST_CASE("a tap between two steps still reaches the next step once") {
  InputState s;
  const alpha::core::PadSlots slots;
  apply_event(s, key(SDL_SCANCODE_SPACE, true), slots);
  apply_event(s, key(SDL_SCANCODE_SPACE, false), slots);
  CHECK(has(held1(s, slots), "Space"));
  CHECK_FALSE(has(held1(s, slots), "Space"));
}

TEST_CASE("gamepad axes arrive normalized for the pad's player; unknown pads are ignored") {
  InputState s;
  alpha::core::PadSlots slots = one_pad();
  slots.connect(9, "pad");
  apply_event(s, axis(SDL_GAMEPAD_AXIS_LEFTX, 32767, 9), slots);
  apply_event(s, axis(SDL_GAMEPAD_AXIS_LEFTY, -32768), slots);
  apply_event(s, axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767), slots);
  apply_event(s, axis(SDL_GAMEPAD_AXIS_LEFTX, 32767, 42), slots);
  apply_event(s, pad(SDL_GAMEPAD_BUTTON_SOUTH, true, 9), slots);
  apply_event(s, pad(SDL_GAMEPAD_BUTTON_NORTH, true, 42), slots);
  const auto frame = consume(s, slots);
  CHECK(frame.connected == 2);
  CHECK(frame.players[0].analog[0] == 0.0f);
  CHECK(frame.players[0].analog[1] == doctest::Approx(-1.0f));
  CHECK(frame.players[0].analog[5] == doctest::Approx(1.0f));
  CHECK(frame.players[1].analog[0] == doctest::Approx(1.0f));
  CHECK(has(frame.players[1].held, "GamepadA"));
  CHECK(frame.players[0].held.none());
}

TEST_CASE("a disconnected pad's player lets go of everything") {
  InputState s;
  alpha::core::PadSlots slots = one_pad();
  apply_event(s, pad(SDL_GAMEPAD_BUTTON_SOUTH, true), slots);
  apply_event(s, axis(SDL_GAMEPAD_AXIS_LEFTX, 32767), slots);
  consume(s, slots);
  release_player(s, *slots.disconnect(7));
  const auto frame = consume(s, slots);
  CHECK(frame.players[0].held.none());
  CHECK(frame.players[0].analog[0] == 0.0f);
}

TEST_CASE("held inputs stay down across steps; repeats and unrelated events are ignored") {
  InputState s;
  const alpha::core::PadSlots slots = one_pad();
  apply_event(s, mouse(SDL_BUTTON_LEFT, true), slots);
  apply_event(s, pad(SDL_GAMEPAD_BUTTON_SOUTH, true), slots);
  apply_event(s, key(SDL_SCANCODE_A, true, true), slots);
  SDL_Event quit{};
  quit.type = SDL_EVENT_QUIT;
  apply_event(s, quit, slots);
  consume(s, slots);
  const auto held = held1(s, slots);
  CHECK(has(held, "MouseLeft"));
  CHECK(has(held, "GamepadA"));
  CHECK_FALSE(has(held, "A"));
  apply_event(s, mouse(SDL_BUTTON_LEFT, false), slots);
  apply_event(s, pad(SDL_GAMEPAD_BUTTON_SOUTH, false), slots);
  CHECK(held1(s, slots).none());
}
