#pragma once

#include <quickjs.h>

namespace alpha::script {

// Natives applying ctx.settings / ctx.locale to the engine: UI translations
// (data-t), UI scale and high contrast, reduced motion, the locale mirror.
void install_settings_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
