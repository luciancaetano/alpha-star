#include "script/text_look.hpp"

#include <memory>
#include <utility>

#include "core/glyph_layout.hpp"
#include "platform/fonts.hpp"
#include "script/spec_fields.hpp"

namespace alpha::script {

namespace {

// The atlas at `pixels`, made once per font and size.
core::FontAtlasPtr atlas_of(ScriptState& s, const core::FontSourcePtr& face, int pixels) {
  auto& cached = s.font_atlases[{face.get(), pixels}];
  if (!cached) cached = face->atlas(pixels);
  return cached;
}

}  // namespace

void lay_out(ScriptState& s, core::ShapeSpec& shape) {
  core::TextLook& look = *shape.text;
  look.face = look.font.empty() ? platform::default_font() : named(s.fonts, look.font, "shape.font", "assets.fonts");
  const core::FontScale scale = core::font_scale(look.face->native(), look.size, 1.0f);
  const core::TextQuads quads = core::layout_text(*atlas_of(s, look.face, scale.pixels), look.text, look.align);
  shape.geometry = core::Rect{quads.box * scale.texel};
  shape.image = nullptr;
  shape.tiles = nullptr;
}

}  // namespace alpha::script
