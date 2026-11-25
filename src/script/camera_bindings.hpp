#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs the scene camera natives (pos/zoom/rotation, follow, shake, bounds, toWorld) on `target`.
void install_camera_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
