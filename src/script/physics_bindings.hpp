#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs physics natives: collision categories, raycasts, area queries,
// forces / impulses / torque / angular velocity / mass.
void install_physics_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
