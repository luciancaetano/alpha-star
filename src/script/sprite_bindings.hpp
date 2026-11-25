#pragma once

#include <quickjs.h>

#include <string>

#include "script/script_state.hpp"

namespace alpha::script {

// Installs image, sprite-sheet, animation and flip natives on `target`.
void install_sprite_bindings(JSContext* ctx, JSValueConst target);

// The image file (relative to the asset dir), decoded once per instance.
core::ImagePtr decoded_image(ScriptState& s, const std::string& file);

}  // namespace alpha::script
