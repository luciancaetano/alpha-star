#pragma once

#include <SDL3/SDL.h>

namespace alpha::devtools {

// What ImGui claimed last frame (io.WantCaptureMouse / WantCaptureKeyboard).
struct Capture {
  bool mouse = false;
  bool keyboard = false;
};

// F1 (not a key repeat), or gamepad Back/Start pressed while the other is held.
bool is_toggle(const SDL_Event& event, bool other_button_held);
// False for the toggle key and for mouse/keyboard events ImGui captures.
bool reaches_game(const SDL_Event& event, Capture capture);

}  // namespace alpha::devtools
