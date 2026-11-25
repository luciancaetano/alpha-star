#pragma once

#include <quickjs.h>

namespace alpha::script {

// Files the game keeps between runs (save slots, settings), in the player's
// preference directory: prefsRead / prefsWrite. Instances without a save
// directory (test runs) never reach them.
void install_prefs_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
