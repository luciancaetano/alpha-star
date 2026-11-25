#pragma once

#include "core/entity_spec.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// Resolves shape.text (required): its font (an assets.fonts name, or the
// built-in font when none is named) and its box at 1 output pixel per game
// pixel, which becomes the geometry. The renderer draws the glyphs at the
// output resolution. Throws naming assets.fonts for an unknown font.
void lay_out(ScriptState& s, core::ShapeSpec& shape);

}  // namespace alpha::script
