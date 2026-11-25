#pragma once

#include "core/entity_spec.hpp"

namespace alpha::render {

// SDL's built-in debug font: 8x8 pixel glyphs, scaled to TextItem::size.
inline constexpr float kGlyphPixels = 8.0f;

struct TextPlacement {
  core::Vec2 top_left;  // in scaled space (divide-by-scale already applied)
  float scale;
};

// Where to draw a line of text so align and vertical centering hold.
TextPlacement place_text(const core::TextItem& item);

}  // namespace alpha::render
