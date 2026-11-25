#include "platform/sdl_input.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace alpha::platform {

namespace {

template <typename Code, std::size_t N>
std::optional<core::InputCode> lookup(const std::array<std::pair<Code, std::string_view>, N>& table,
                                      Code code) {
  for (const auto& [sdl, name] : table) {
    if (sdl == code) return core::parse_input(name);
  }
  return std::nullopt;
}

constexpr std::array<std::pair<SDL_Scancode, std::string_view>, 13> kKeys{{
    {SDL_SCANCODE_SPACE, "Space"},     {SDL_SCANCODE_RETURN, "Enter"},
    {SDL_SCANCODE_ESCAPE, "Escape"},   {SDL_SCANCODE_TAB, "Tab"},
    {SDL_SCANCODE_BACKSPACE, "Backspace"}, {SDL_SCANCODE_LEFT, "Left"},
    {SDL_SCANCODE_RIGHT, "Right"},     {SDL_SCANCODE_UP, "Up"},
    {SDL_SCANCODE_DOWN, "Down"},       {SDL_SCANCODE_LSHIFT, "LeftShift"},
    {SDL_SCANCODE_RSHIFT, "RightShift"}, {SDL_SCANCODE_LCTRL, "LeftCtrl"},
    {SDL_SCANCODE_RCTRL, "RightCtrl"},
}};

constexpr std::array<std::pair<int, std::string_view>, 3> kMouse{{
    {SDL_BUTTON_LEFT, "MouseLeft"}, {SDL_BUTTON_RIGHT, "MouseRight"},
    {SDL_BUTTON_MIDDLE, "MouseMiddle"},
}};

constexpr std::array<std::pair<SDL_GamepadButton, std::string_view>, 12> kGamepad{{
    {SDL_GAMEPAD_BUTTON_SOUTH, "GamepadA"},       {SDL_GAMEPAD_BUTTON_EAST, "GamepadB"},
    {SDL_GAMEPAD_BUTTON_WEST, "GamepadX"},        {SDL_GAMEPAD_BUTTON_NORTH, "GamepadY"},
    {SDL_GAMEPAD_BUTTON_START, "GamepadStart"},   {SDL_GAMEPAD_BUTTON_BACK, "GamepadBack"},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, "GamepadUp"},    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, "GamepadDown"},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, "GamepadLeft"}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, "GamepadRight"},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "GamepadLB"},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "GamepadRB"},
}};

// Letters and digits are contiguous in SDL and named by their character.
std::optional<core::InputCode> alphanumeric(SDL_Scancode code) {
  if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z) {
    const char name[2] = {static_cast<char>('A' + (code - SDL_SCANCODE_A)), '\0'};
    return core::parse_input(name);
  }
  if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_0) {
    const char name[2] = {static_cast<char>('0' + (code - SDL_SCANCODE_1 + 1) % 10), '\0'};
    return core::parse_input(name);
  }
  return std::nullopt;
}

void set(InputState& state, std::optional<core::InputCode> code, bool is_down) {
  if (!code) return;
  state.down.set(*code, is_down);
  if (is_down) state.pressed.set(*code);
}

}  // namespace

std::optional<core::InputCode> from_scancode(SDL_Scancode code) {
  if (const auto c = alphanumeric(code)) return c;
  return lookup(kKeys, code);
}

std::optional<core::InputCode> from_mouse(Uint8 button) { return lookup(kMouse, static_cast<int>(button)); }

std::optional<core::InputCode> from_gamepad(Uint8 button) { return lookup(kGamepad, static_cast<SDL_GamepadButton>(button)); }

void apply_event(InputState& state, const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      if (!event.key.repeat) set(state, from_scancode(event.key.scancode), event.key.down);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      set(state, from_mouse(event.button.button), event.button.down);
      break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
      set(state, from_gamepad(event.gbutton.button), event.gbutton.down);
      break;
    default:
      break;
  }
}

core::InputSet consume(InputState& state) {
  const core::InputSet held = state.down | state.pressed;
  state.pressed.reset();
  return held;
}

}  // namespace alpha::platform
