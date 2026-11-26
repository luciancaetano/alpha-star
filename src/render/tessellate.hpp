#pragma once

#include <span>
#include <vector>

#include "core/entity_spec.hpp"

namespace alpha::render {

struct Vertex {
  core::Vec2 pos;
  core::Color color;
  core::Vec2 uv{};  // texture coordinates (0..1); unused by plain shapes
};

inline constexpr float kOutlineWidth = 2.0f;
inline constexpr int kMinSegments = 12;
// Fewest segments of a smooth particle circle: the 0.3 px error rule alone (a 1 px
// radius needs 4), so tiny particles stay cheap; their feather hides the corners.
inline constexpr int kMinParticleSegments = 4;

// Segments of a circle `radius_px` output pixels wide: enough that the polygon
// strays under ~0.3 px from the true circle, clamped to min_segments..128.
int circle_segments(float radius_px, int min_segments = kMinSegments);

// Turns shapes into a triangle list (3 vertices per triangle), in order.
// `pixel` is the size of one output pixel in world units (1 / (zoom * window
// scale)): it sets circle segments and the width of a smooth shape's feather.
// Outlined shapes emit the outline (grown by kOutlineWidth) before the fill.
// Smooth shapes (ShapeSpec::smooth) fan to an edge pulled in by half a pixel,
// then a ring out to half a pixel beyond it whose outer vertices have alpha 0.
std::vector<Vertex> tessellate(std::span<const core::DrawItem> items, float pixel = 1.0f);
// Appends a filled circle of `segments` triangles around `center`.
void fan(core::Vec2 center, float radius, int segments, core::Color color, std::vector<Vertex>& out);
// Appends one plain shape's triangles.
void tessellate(const core::DrawItem& item, std::vector<Vertex>& out, float pixel = 1.0f);
// Appends one smooth (feathered edge) particle: `g` already scaled to its size, unrotated.
void smooth_particle(const core::Geometry& g, core::Vec2 pos, core::Color color, float pixel, std::vector<Vertex>& out);

}  // namespace alpha::render
