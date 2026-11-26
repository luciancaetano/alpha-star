#pragma once

#include <quickjs.h>

#include <string>

#include "core/image.hpp"
#include "core/touch_button.hpp"
#include "core/touch_stick.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// An image of the game's assets for a touch control skin: an assets.images
// name, or "sheet:frame" (frame index of an assets.sheets sheet). A missing
// one is an error naming the closest declared name ("did you mean ...").
core::Picture skin_picture(const ScriptState& s, const std::string& name, const std::string& what);
// config.touchSticks.<name>.skin / config.touchButtons.<name>.skin.
core::StickSkin stick_skin(JSContext* ctx, const ScriptState& s, JSValueConst v, const std::string& what);
core::ButtonSkin button_skin(JSContext* ctx, const ScriptState& s, JSValueConst v, const std::string& what);

}  // namespace alpha::script
