#include "devtools/routing.hpp"

namespace alpha::devtools {

namespace {

bool is_gamepad_toggle(const SDL_Event& event, bool other_button_held) {
  const Uint8 b = event.gbutton.button;
  return other_button_held && (b == SDL_GAMEPAD_BUTTON_BACK || b == SDL_GAMEPAD_BUTTON_START);
}

bool is_key(Uint32 type) { return type == SDL_EVENT_KEY_DOWN || type == SDL_EVENT_KEY_UP; }

bool is_mouse(Uint32 type) {
  return type == SDL_EVENT_MOUSE_MOTION || type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
         type == SDL_EVENT_MOUSE_BUTTON_UP || type == SDL_EVENT_MOUSE_WHEEL;
}

}  // namespace

bool is_toggle(const SDL_Event& event, bool other_button_held) {
  if (event.type == SDL_EVENT_KEY_DOWN) return event.key.scancode == SDL_SCANCODE_F12 && !event.key.repeat;
  if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) return is_gamepad_toggle(event, other_button_held);
  return false;
}

bool reaches_game(const SDL_Event& event, Capture capture) {
  if (is_key(event.type) && event.key.scancode == SDL_SCANCODE_F12) return false;
  const bool keyboard = is_key(event.type) || event.type == SDL_EVENT_TEXT_INPUT;
  if (keyboard && capture.keyboard) return false;
  return !(is_mouse(event.type) && capture.mouse);
}

}  // namespace alpha::devtools
