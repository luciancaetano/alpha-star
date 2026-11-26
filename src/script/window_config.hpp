#pragma once

#include <quickjs.h>

#include "core/window.hpp"
#include "script/js_value.hpp"

namespace alpha::script {

// config.window of the game config `info`: { fullscreen?, aspect?, scale?,
// resizable?, altEnter?, borderless?, alwaysOnTop?, maximized?, vsync? }.
// The removed `scaling` fails naming its replacement.
// obj[name] as a boolean (truthiness), or `fallback` when absent.
bool flag_or(JSContext* ctx, const Value& obj, const char* name, bool fallback);

core::WindowConfig window_config(JSContext* ctx, const Value& info);

}  // namespace alpha::script
