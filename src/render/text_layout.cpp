#include "render/text_layout.hpp"

namespace alpha::render {

TextPlacement place_text(const core::TextItem& item) {
  const float scale = item.size / kGlyphPixels;
  const float width = static_cast<float>(item.text.size()) * item.size;
  float left = item.pos.x;
  if (item.align == core::Align::Center) left -= width * 0.5f;
  if (item.align == core::Align::Right) left -= width;
  const float top = item.pos.y - item.size * 0.5f;
  return {{left / scale, top / scale}, scale};
}

}  // namespace alpha::render
