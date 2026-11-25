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
void image_triangles(const core::DrawItem& item, const core::Box& area, std::vector<Vertex>& out);

}  // namespace alpha::render
