#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs natives for joints, parent/child links, entity scale and entity data.
void install_joint_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
