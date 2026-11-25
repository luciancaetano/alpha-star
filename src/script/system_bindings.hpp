#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs non-world natives (config, actions, random, timers, save) on `target`.
void install_system_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
