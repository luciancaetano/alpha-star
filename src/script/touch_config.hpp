#pragma once

#include <quickjs.h>

#include <span>
#include <string>
#include <vector>

#include "core/touch_button.hpp"
#include "core/touch_stick.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// config.touchSticks / config.touchButtons (an object of name -> spec, or
// undefined), validated: errors name the field and what it accepts
// ("config.touchSticks.move.deadzone must be in [0, 0.9], got 1.2").
// Skin images resolve against the game's assets (touch_skin).
std::vector<core::StickConfig> touch_sticks(JSContext* ctx, const ScriptState& s, JSValueConst sticks);
std::vector<core::ButtonConfig> touch_buttons(JSContext* ctx, const ScriptState& s, JSValueConst buttons);

// Throws unless `obj` is an object whose keys are all in `fields` (the error lists them).
void only_fields(JSContext* ctx, JSValueConst obj, const std::string& what, std::span<const char* const> fields);
// obj[name] in [lo, hi] when present, else `fallback`.
float number_in(JSContext* ctx, JSValueConst obj, const char* name, const std::string& what, double lo, double hi,
                float fallback);

}  // namespace alpha::script
