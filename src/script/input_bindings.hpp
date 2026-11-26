#pragma once

#include <quickjs.h>

#include <cstddef>

#include "core/touch_stick.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// Installs the input natives on `target`: bind (actions, axes), deadzone,
// action, axis, players, rumble, mouse, touches, touchControls.
void install_input_bindings(JSContext* ctx, JSValueConst target);

// A player argument from the runtime: 0 = any player (core::kAnyPlayer),
// 1..kMaxPlayers = that player (returned 0-based). Throws Error otherwise.
std::size_t player_arg(JSContext* ctx, JSValueConst v);

// Where the game's touch controls live now: the safe area of what the screen
// shows (config.size before the first frame).
core::TouchScreen touch_screen(const ScriptState& s);

}  // namespace alpha::script
