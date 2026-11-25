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

constexpr std::array<std::pair<SDL_GamepadButton, std::string_view>, 14> kGamepad{{
    {SDL_GAMEPAD_BUTTON_SOUTH, "GamepadA"},       {SDL_GAMEPAD_BUTTON_EAST, "GamepadB"},
    {SDL_GAMEPAD_BUTTON_WEST, "GamepadX"},        {SDL_GAMEPAD_BUTTON_NORTH, "GamepadY"},
    {SDL_GAMEPAD_BUTTON_START, "GamepadStart"},   {SDL_GAMEPAD_BUTTON_BACK, "GamepadBack"},
    {SDL_GAMEPAD_BUTTON_DPAD_UP, "GamepadUp"},    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, "GamepadDown"},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, "GamepadLeft"}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, "GamepadRight"},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "GamepadLB"},
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "GamepadRB"},
    {SDL_GAMEPAD_BUTTON_LEFT_STICK, "GamepadLS"},  {SDL_GAMEPAD_BUTTON_RIGHT_STICK, "GamepadRS"},
}};

// SDL axis order (LEFTX, LEFTY, RIGHTX, RIGHTY, LEFT_TRIGGER, RIGHT_TRIGGER)
// is core::Analog's order; SDL reports -32768..32767 (triggers 0..32767).
float normalized(Sint16 value) {
  return static_cast<float>(value) / (value < 0 ? 32768.0f : 32767.0f);
}

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

void set(InputState& state, std::size_t player, std::optional<core::InputCode> code, bool is_down) {
  if (!code) return;
  state.down[player].set(*code, is_down);
  if (is_down) state.pressed[player].set(*code);
}

void pad_button(InputState& state, const SDL_GamepadButtonEvent& e, const core::PadSlots& slots) {
  if (const auto player = slots.slot_of(e.which)) set(state, *player, from_gamepad(e.button), e.down);
}

void pad_axis(InputState& state, const SDL_GamepadAxisEvent& e, const core::PadSlots& slots) {
  const auto player = slots.slot_of(e.which);
  if (player && e.axis < core::kAnalogCount) state.analog[*player][e.axis] = normalized(e.value);
}

}  // namespace

std::optional<core::InputCode> from_scancode(SDL_Scancode code) {
  if (const auto c = alphanumeric(code)) return c;
  return lookup(kKeys, code);
}

std::optional<core::InputCode> from_mouse(Uint8 button) { return lookup(kMouse, static_cast<int>(button)); }

std::optional<core::InputCode> from_gamepad(Uint8 button) { return lookup(kGamepad, static_cast<SDL_GamepadButton>(button)); }

namespace {

// Keyboard and mouse (always player 1); false for any other event.
bool desktop(InputState& state, const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      if (!event.key.repeat) set(state, 0, from_scancode(event.key.scancode), event.key.down);
      return true;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      set(state, 0, from_mouse(event.button.button), event.button.down);
      return true;
    case SDL_EVENT_MOUSE_MOTION:
      state.mouse = {event.motion.x, event.motion.y};
      return true;
    default:
      return false;
  }
}

}  // namespace

void apply_event(InputState& state, const SDL_Event& event, const core::PadSlots& slots) {
  if (desktop(state, event)) return;
  switch (event.type) {
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
      pad_button(state, event.gbutton, slots);
      break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
      pad_axis(state, event.gaxis, slots);
      break;
    default:
      break;
  }
}

void release_player(InputState& state, std::size_t player) {
  state.down[player].reset();
  state.analog[player] = {};
}

core::InputFrame consume(InputState& state, const core::PadSlots& slots) {
  core::InputFrame frame;
  for (std::size_t p = 0; p < core::kMaxPlayers; ++p) {
    frame.players[p] = {state.down[p] | state.pressed[p], state.analog[p]};
    state.pressed[p].reset();
  }
  frame.connected = slots.players();
  frame.mouse = state.mouse;
  return frame;
}

}  // namespace alpha::platform
