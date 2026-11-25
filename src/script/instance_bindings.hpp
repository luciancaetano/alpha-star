#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs the natives that create and select game instances (simulate) and
// drive one headlessly: instanceCreate/Use/Destroy, holdActions, tagCounts, stepBudget.
void install_instance_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
