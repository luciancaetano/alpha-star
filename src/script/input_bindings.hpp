#pragma once

#include <quickjs.h>

#include <cstddef>

namespace alpha::script {

// Installs the input natives on `target`: bind (actions, axes), deadzone,
// action, axis, players, rumble.
void install_input_bindings(JSContext* ctx, JSValueConst target);

// A player argument from the runtime: 0 = any player (core::kAnyPlayer),
// 1..kMaxPlayers = that player (returned 0-based). Throws Error otherwise.
std::size_t player_arg(JSContext* ctx, JSValueConst v);

}  // namespace alpha::script
