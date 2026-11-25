#pragma once

#include <vector>

#include "core/lighting.hpp"
#include "core/vec2.hpp"
#include "render/tessellate.hpp"

// Triangle lists (screen space, game pixels) the compositor sends to SDL:
// shapes made of vertex colors, so they draw the same on every SDL renderer.
namespace alpha::render {

// A light: full color at the center, ~35% at half the radius, black at the
// edge (meant for additive blending into a lightmap).
void light_triangles(const core::LightDisc& disc, std::vector<Vertex>& out);
// A black overlay over `size` whose alpha grows from 0 inside `radius`
// (0..1 of the half diagonal) to `strength` at the corners.
void vignette_triangles(core::Vec2 size, float strength, float radius, std::vector<Vertex>& out);
// A 1 px black line of alpha `strength` on every other row of `size`.
void scanline_triangles(core::Vec2 size, float strength, std::vector<Vertex>& out);

}  // namespace alpha::render
