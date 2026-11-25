#include "script/text_look.hpp"

#include <cmath>
#include <memory>
#include <utility>

#include "core/glyph_layout.hpp"
#include "platform/glyphs.hpp"
#include "script/spec_fields.hpp"

namespace alpha::script {

namespace {

core::FontAtlasPtr atlas_of(ScriptState& s, const std::string& font, float size) {
  const std::filesystem::path file = named(s.fonts, font, "shape.font", "assets.fonts");
  const auto pixels = static_cast<int>(std::lround(size));
  auto& cached = s.font_atlases[{font, pixels}];
  if (cached) return cached;
  cached = platform::rasterize_font(file, pixels);
  if (s.smooth_fonts.contains(font)) {  // pixelMode: false
    auto image = std::make_shared<core::Image>(*cached->image);
    image->smooth = true;
    auto atlas = std::make_shared<core::FontAtlas>(*cached);
    atlas->image = std::move(image);
    cached = std::move(atlas);
  }
  return cached;
}

}  // namespace

void lay_out(ScriptState& s, core::ShapeSpec& shape) {
  const core::TextLook& look = *shape.text;
  if (look.font.empty()) {
    shape.geometry = core::Rect{core::debug_text_box(look.text, look.size, look.align)};
    shape.image = nullptr;
    shape.tiles = nullptr;
    return;
  }
  const core::FontAtlasPtr font = atlas_of(s, look.font, look.size);
  core::TextQuads quads = core::layout_text(*font, look.text, look.align);
  shape.geometry = core::Rect{quads.box};
  shape.image = font->image;
  shape.tiles = std::make_shared<const core::TileGrid>(std::move(quads.grid));
}

}  // namespace alpha::script
