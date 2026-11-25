#pragma once

#include <quickjs.h>

#include "script/script_state.hpp"

namespace alpha::script {

// Installs entity/world natives (spawn, find, transforms, step) on `target`.
void install_world_bindings(JSContext* ctx, JSValueConst target);

// Shared by the natives that act on entities: the scene's world (throws
// before the first scene) and an entity id argument.
ecs::World& world_of(ScriptState& s);
ecs::EntityId id_arg(JSContext* ctx, JSValueConst v);

}  // namespace alpha::script
