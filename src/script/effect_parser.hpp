#pragma once

#include <quickjs.h>

#include <string>

#include "core/entity_spec.hpp"
#include "script/script_state.hpp"

// Spawn spec parts added by step 9: looks (tint/alpha/blend), world text,
// layers and particle emitters. Errors name the field ("spawn: emitter.max ...").
namespace alpha::script {

// "alpha" | "add" | "multiply".
core::Blend parse_blend(JSContext* ctx, JSValueConst v, const std::string& path);
// obj.tint?, obj.alpha? (0..1), obj.blend? onto `shape`; `path` prefixes field names.
void parse_look(JSContext* ctx, JSValueConst obj, const std::string& path, core::ShapeSpec& shape);
// { text, font?, size?, color?, align? }: a laid-out text shape.
core::ShapeSpec parse_text_shape(JSContext* ctx, JSValueConst obj, ScriptState& s);
// A config.layers name (absent: the default layer).
core::Layer parse_layer(JSContext* ctx, JSValueConst obj, const ScriptState& s);
// { rate?, burst?, lifetime?, speed?, angle?, gravity?, size?, color?, shape?, max?, oneShot?, tint?, alpha?, blend? }
core::EmitterSpec parse_emitter(JSContext* ctx, JSValueConst obj, const ScriptState& s);

}  // namespace alpha::script
