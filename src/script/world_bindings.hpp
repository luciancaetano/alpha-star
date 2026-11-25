#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs entity/world natives (spawn, find, transforms, step) on `target`.
void install_world_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
