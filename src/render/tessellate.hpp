#pragma once

#include <span>
#include <vector>

#include "core/entity_spec.hpp"

namespace alpha::render {

struct Vertex {
  core::Vec2 pos;
  core::Color color;
};

inline constexpr int kCircleSegments = 32;
inline constexpr float kOutlineWidth = 2.0f;

// Turns shapes into a triangle list (3 vertices per triangle), in order.
// Outlined shapes emit the outline (grown by kOutlineWidth) before the fill.
std::vector<Vertex> tessellate(std::span<const core::DrawItem> items);

}  // namespace alpha::render
