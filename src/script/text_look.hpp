#pragma once

#include "core/entity_spec.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// Lays out shape.text (required): with a font, glyph quads of its atlas
// (rasterized once per font and pixel size, cached in `s`) as image + tiles;
// without one, the debug-font box (drawn by the renderer's fallback). The
// geometry becomes the text box. Throws naming assets.fonts for an unknown font.
void lay_out(ScriptState& s, core::ShapeSpec& shape);

}  // namespace alpha::script
