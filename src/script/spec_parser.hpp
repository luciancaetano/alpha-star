#pragma once

#include <quickjs.h>


#include "core/entity_spec.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// Parses a script entity spec { tags, pos, rotation?, z?, layer?, shape?,
// body?, emitter? }. Throws core::Error naming the bad field, e.g.
// `spawn: shape.color must be ...`. Tag names are interned into s.tags;
// image, sheet, font and layer names come from the game's assets and config.
core::EntitySpec parse_entity_spec(JSContext* ctx, JSValueConst spec, ScriptState& s);

}  // namespace alpha::script
