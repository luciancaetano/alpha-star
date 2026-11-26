#include "render/quads.hpp"

#include <array>
#include <cmath>
#include <utility>

#include "core/particles.hpp"

namespace alpha::render {

namespace {

constexpr core::Color kWhite{255, 255, 255, 255};
constexpr int kParticleSides = 8;

struct Uv {
  core::Vec2 min;
  core::Vec2 max;
};

// src in texture space; an empty src is the whole image.
Uv uv_of(const core::Image& image, core::Region src, bool flip_x, bool flip_y) {
  if (src.w <= 0 || src.h <= 0) src = {0, 0, image.width, image.height};
  const auto w = static_cast<float>(image.width);
  const auto h = static_cast<float>(image.height);
  Uv uv{{static_cast<float>(src.x) / w, static_cast<float>(src.y) / h},
        {static_cast<float>(src.x + src.w) / w, static_cast<float>(src.y + src.h) / h}};
  if (flip_x) std::swap(uv.min.x, uv.max.x);
  if (flip_y) std::swap(uv.min.y, uv.max.y);
  return uv;
}

// corners: top-left, top-right, bottom-right, bottom-left.
void quad(const std::array<core::Vec2, 4>& corners, const Uv& uv, core::Color color, std::vector<Vertex>& out) {
  const std::array<core::Vec2, 4> uvs{uv.min, core::Vec2{uv.max.x, uv.min.y}, uv.max, core::Vec2{uv.min.x, uv.max.y}};
  for (const std::size_t i : {0u, 1u, 2u, 0u, 2u, 3u}) out.push_back({corners[i], color, uvs[i]});
}

void box(core::Vec2 a, core::Vec2 b, const Uv& uv, core::Color color, std::vector<Vertex>& out) {
  quad({a, core::Vec2{b.x, a.y}, b, core::Vec2{a.x, b.y}}, uv, color, out);
}

core::Color image_color(const core::ShapeSpec& shape) {
  return core::shade(shape.text ? shape.fill : kWhite, shape.tint, shape.alpha);
}

void sprite(const core::DrawItem& item, std::vector<Vertex>& out) {
  const core::Vec2 h = std::get<core::Rect>(item.shape.geometry).size * 0.5f;
  const auto at = [&](float x, float y) { return item.pos + core::rotated({x, y}, item.rotation); };
  const auto& shape = item.shape;
  quad({at(-h.x, -h.y), at(h.x, -h.y), at(h.x, h.y), at(-h.x, h.y)},
       uv_of(*shape.image, shape.src, shape.flip_x, shape.flip_y), image_color(shape), out);
}

void tiles(const core::DrawItem& item, const core::Box& area, std::vector<Vertex>& out) {
  const core::TileGrid& grid = *item.shape.tiles;
  const core::Vec2 origin = item.pos - std::get<core::Rect>(item.shape.geometry).size * 0.5f;
  const core::Color color = image_color(item.shape);
  for (const core::TileQuad& t : grid.tiles) {
    const core::Vec2 a = origin + t.pos;
    const core::Vec2 b = a + (grid.tile.x > 0 ? grid.tile : core::Vec2{static_cast<float>(t.src.w), static_cast<float>(t.src.h)});
    if (!core::overlaps({a, b}, area)) continue;
    box(a, b, uv_of(*item.shape.image, t.src, false, false), color, out);
  }
}

}  // namespace

void text_triangles(const core::DrawItem& item, const GlyphRun& run, std::vector<Vertex>& out) {
  const core::Color color = image_color(item.shape);
  for (const core::TileQuad& t : run.quads.grid.tiles) {
    const core::Vec2 a = run.origin + t.pos * run.unit;
    const core::Vec2 size{static_cast<float>(t.src.w), static_cast<float>(t.src.h)};
    box(a, a + size * run.unit, uv_of(run.image, t.src, false, false), color, out);
  }
}

namespace {

float radius_of(const core::Geometry& g) {
  if (const auto* c = std::get_if<core::Circle>(&g)) return c->radius;
  const core::Vec2 size = std::get<core::Rect>(g).size;
  return 0.5f * std::sqrt(size.x * size.x + size.y * size.y);
}

}  // namespace

core::Box bounds_of(const core::DrawItem& item) {
  const float r = radius_of(item.shape.geometry) + (item.shape.outline ? kOutlineWidth : 0.0f);
  return {item.pos - core::Vec2{r, r}, item.pos + core::Vec2{r, r}};
}

void image_triangles(const core::DrawItem& item, const core::Box& area, std::vector<Vertex>& out) {
  if (item.shape.tiles) tiles(item, area, out);
  else sprite(item, out);
}

namespace {

core::Geometry sized(bool round, float size) {
  if (round) return core::Circle{size * 0.5f};
  return core::Rect{{size, size}};
}

}  // namespace

void particle_triangles(const core::DrawItem& item, const core::Box& area, std::vector<Vertex>& out, float pixel) {
  const core::ShapeSpec& look = item.shape;
  const bool round = !look.image && std::holds_alternative<core::Circle>(look.geometry);
  const bool smooth = look.smooth && !look.image;
  const Uv uv = look.image ? uv_of(*look.image, look.src, false, false) : Uv{};
  for (const core::ParticleQuad& p : item.particles) {
    const core::Vec2 h{p.size * 0.5f, p.size * 0.5f};
    if (!core::overlaps({p.pos - h, p.pos + h}, area)) continue;
    if (smooth) smooth_particle(sized(round, p.size), p.pos, p.color, pixel, out);
    else if (round) fan(p.pos, h.x, kParticleSides, p.color, out);
    else box(p.pos - h, p.pos + h, uv, p.color, out);
  }
}

}  // namespace alpha::render
