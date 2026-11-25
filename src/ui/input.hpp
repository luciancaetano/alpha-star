#pragma once

#include <SDL3/SDL.h>

#include "ui/ui.hpp"

namespace alpha::ui {

// Feeds one SDL event to the game UI and says whether the UI consumed it.
// A consumed event must not also drive game actions. The rule:
//  - mouse button: consumed when it lands on an element with a ui.on handler
//    (or inside one), or on a form control (input, textarea, select);
//    touch arrives as mouse events (SDL's default), so the same rule holds;
//  - key or gamepad button down: consumed when the UI acted on it (focus moved,
//    a focused element was clicked by Enter/Space/A, Escape/B ran a "back"
//    handler) or a text field has focus;
//  - left stick: consumed when it moved focus;
//  - wheel: consumed when it scrolled something;
//  - motion, key/button up, everything else: not consumed.
// Gamepad: D-pad and left stick (PadNav: deadzone, hold repeat) move focus
// down/right = next focusable, up/left = previous; A = Enter; B = Escape (back).
// Any gamepad event makes the gamepad the last device (autofocus on show).
// Times come from event timestamps; call Ui::tick every frame for repeats.
// Mouse coordinates must already be the game's logical pixels
// (SDL_ConvertEventToRenderCoordinates).
bool consumes(Ui& ui, const SDL_Event& event);

}  // namespace alpha::ui
