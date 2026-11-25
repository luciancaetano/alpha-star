#pragma once

#include <quickjs.h>

#include "core/lighting.hpp"

// ctx.postfx natives (postfxPut/Remove/List/Clear), config.lighting
// (setLighting) and the spawn `light` field. Render-only state.
namespace alpha::script {

void install_postfx_bindings(JSContext* ctx, JSValueConst target);
// { radius, color?, intensity? }. Errors name the field ("spawn: light.radius ...").
core::LightSpec parse_light(JSContext* ctx, JSValueConst obj);

}  // namespace alpha::script
