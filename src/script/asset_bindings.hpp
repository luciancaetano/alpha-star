#pragma once

#include <quickjs.h>

namespace alpha::script {

// Installs asset and audio natives (load/synth sounds, load images, play,
// tone, stop, volume) on `target`.
void install_asset_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
