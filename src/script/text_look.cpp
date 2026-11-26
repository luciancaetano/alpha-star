#include "script/text_look.hpp"

#include <memory>
#include <utility>

#include "core/glyph_layout.hpp"
#include "platform/fonts.hpp"
#include "script/spec_fields.hpp"

namespace alpha::script {

void lay_out(ScriptState& s, core::ShapeSpec& shape) {
  core::TextLook& look = *shape.text;
  look.face = look.font.empty() ? platform::default_font() : named(s.fonts, look.font, "shape.font", "assets.fonts");
  const core::FontScale scale = core::font_scale(look.face->native(), look.size, 1.0f);
  const core::ShapedText shaped = look.face->shape({look.text, scale.pixels, look.dir, look.width / scale.texel});
  const core::TextQuads quads = core::layout_text(shaped, look.align);
  shape.geometry = core::Rect{quads.box * scale.texel};
  shape.image = nullptr;
  shape.tiles = nullptr;
}

}  // namespace alpha::script
