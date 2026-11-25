#pragma once

#include <quickjs.h>

#include <map>
#include <string>

#include "core/entity_spec.hpp"
#include "core/tags.hpp"

namespace alpha::script {

// Parses a script entity spec { tags, pos, rotation?, z?, shape?, body? }.
// Throws core::Error naming the bad field, e.g. `spawn: shape.color must be ...`.
// Tag names are interned into `tags`; `shape.image` / `shape.sheet` names
// come from the game's assets.
struct SpriteAssets {
  const core::PictureTable& images;
  const core::SheetTable& sheets;
};
core::EntitySpec parse_entity_spec(JSContext* ctx, JSValueConst spec, core::TagRegistry& tags,
                                   const SpriteAssets& assets);

}  // namespace alpha::script
