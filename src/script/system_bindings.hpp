#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs non-world natives (config, random, timers, save, log) on `target`.
void install_system_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
