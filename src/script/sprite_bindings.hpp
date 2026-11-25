#pragma once

#include <quickjs.h>

#include <string>

#include "script/script_state.hpp"

namespace alpha::script {

// Installs image, sprite-sheet, animation and flip natives on `target`.
void install_sprite_bindings(JSContext* ctx, JSValueConst target);

// The image file (relative to the asset dir), decoded once per instance.
core::ImagePtr decoded_image(ScriptState& s, const std::string& file, core::Filter filter);
// argv[i] as pixelMode: true / undefined = crisp (nearest), false = smooth (linear).
bool smooth_arg(JSContext* ctx, JSValueConst v);
// argv[i] as an image filter: "nearest" (or undefined), "linear" or "sharp".
core::Filter filter_arg(JSContext* ctx, JSValueConst v);

}  // namespace alpha::script
