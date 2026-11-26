#pragma once

#include <vector>

#include "core/camera.hpp"
#include "core/entity_spec.hpp"
#include "core/glyph_layout.hpp"
#include "render/tessellate.hpp"

namespace alpha::render {

// World box an item can touch (rotation and outline included): what culling tests.
core::Box bounds_of(const core::DrawItem& item);

// Appends the textured triangles of an image item in world coordinates: one
// quad for a sprite (src rect, flips, rotation), one per tile of a tile layer
// whose cell overlaps `area` (the others are culled here).
// Cells are drawn at grid.tile (a zero tile: each at its src size, glyphs). Colors: white (text: fill)
// times the shape's tint and alpha.
void image_triangles(const core::DrawItem& item, const core::Box& area, std::vector<Vertex>& out);
// A line of glyphs ready to draw: `quads` in atlas pixels of `image`, `unit`
// world units per atlas pixel, the line's top-left at `origin`.
struct GlyphRun {
  const core::TextQuads& quads;
  const core::Image& image;
  core::Vec2 origin;
  float unit = 1.0f;
};
// Appends the glyph quads of a text item. Color: the text's fill times tint and alpha.
void text_triangles(const core::DrawItem& item, const GlyphRun& run, std::vector<Vertex>& out);
// Appends one quad (image or square) or octagon (circle) per particle that
// overlaps `area`, scaled to the particle's size, in the particle's color.
// Smooth circle/rect looks (ShapeSpec::smooth) get the shapes' feathered edge,
// `pixel` world units wide (one output pixel); image looks follow their filter.
void particle_triangles(const core::DrawItem& item, const core::Box& area, std::vector<Vertex>& out, float pixel = 1.0f);

}  // namespace alpha::render
