#pragma once

#include <quickjs.h>

#include <map>
#include <string>

#include "core/entity_spec.hpp"
#include "core/tags.hpp"

namespace alpha::script {

// Parses a script entity spec { tags, pos, rotation?, z?, shape?, body? }.
// Throws core::Error naming the bad field, e.g. `spawn: shape.color must be ...`.
// Tag names are interned into `tags`; `shape.image` names come from `images`.
using ImageTable = std::map<std::string, core::ImagePtr, std::less<>>;
core::EntitySpec parse_entity_spec(JSContext* ctx, JSValueConst spec, core::TagRegistry& tags,
                                   const ImageTable& images);

}  // namespace alpha::script
