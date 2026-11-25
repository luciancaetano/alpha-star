#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs the game UI natives (documents, fonts, show/hide/set/on, clicks) on `target`.
void install_ui_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
