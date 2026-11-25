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

inline constexpr int kCircleSegments = 32;
inline constexpr float kOutlineWidth = 2.0f;

// Turns shapes into a triangle list (3 vertices per triangle), in order.
// Outlined shapes emit the outline (grown by kOutlineWidth) before the fill.
std::vector<Vertex> tessellate(std::span<const core::DrawItem> items);
// Appends a filled circle of `segments` triangles around `center`.
void fan(core::Vec2 center, float radius, int segments, core::Color color, std::vector<Vertex>& out);
// Appends one plain shape's triangles.
void tessellate(const core::DrawItem& item, std::vector<Vertex>& out);

}  // namespace alpha::render
