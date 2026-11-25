#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs the natives of looks (tint/alpha/blend/text), layers, particles
// and the window (ctx.window) on `target`.
void install_effect_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
