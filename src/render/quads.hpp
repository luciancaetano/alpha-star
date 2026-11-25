#pragma once

#include <vector>

#include "core/camera.hpp"
#include "core/entity_spec.hpp"
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
// Appends one quad (image or square) or octagon (circle) per particle that
// overlaps `area`, scaled to the particle's size, in the particle's color.
void particle_triangles(const core::DrawItem& item, const core::Box& area, std::vector<Vertex>& out);

}  // namespace alpha::render
