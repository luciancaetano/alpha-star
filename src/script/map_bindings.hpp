#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs the tilemap natives (text asset reading, tile layers, cell merging) on `target`.
// Tiled JSON itself is parsed by the runtime (JSON.parse); see runtime.js maps.
void install_map_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
