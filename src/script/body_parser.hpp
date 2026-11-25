#pragma once

#include <quickjs.h>

#include <cstdint>
#include <string>
#include <vector>

#include "core/entity_spec.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// { circle: r } or { rect: [w, h] }; `path` names the field in errors.
core::Geometry parse_geometry(JSContext* ctx, JSValueConst obj, const std::string& path);

// spawn's `body`: { type, collider: { circle | rect, sensor?, friction?,
// restitution?, density?, oneWay? }, velocity?, fixedRotation?, gravityScale?,
// bullet?, category?, collidesWith? }. Category names come from
// config.collision.categories (s.categories).
core::BodySpec parse_body(JSContext* ctx, JSValueConst obj, const ScriptState& s);

// Bits of the named categories (bit 0 = no category, names from bit 1).
// Throws "<what> "x" is not in config.collision.categories; have: ...".
std::uint64_t category_bits(const ScriptState& s, const std::vector<std::string>& names, const std::string& what);

}  // namespace alpha::script
