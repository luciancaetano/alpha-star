#include "ecs/effects.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <tuple>
#include <utility>

#include "core/error.hpp"
#include "core/particles.hpp"
#include "ecs/components.hpp"

namespace alpha::ecs {

namespace {

// Blends angles the short way around (350 -> 10 passes through 360, not 180).
float blend_angle(float from, float to, float t) {
  const float delta = std::remainder(to - from, 360.0f);
  return from + delta * t;
}

core::DrawItem blended(const core::ShapeSpec& shape, const Previous& p, const Transform& t, float blend) {
  return {shape, p.pos + (t.pos - p.pos) * blend, blend_angle(p.rotation, t.rotation, blend)};
}

// Sort key: layer z, entity z, then order = creation * 2 + (0 shape, 1 particles).
using Keyed = std::tuple<int, int, std::uint64_t, core::DrawItem>;

void add(std::vector<Keyed>& out, const Layered& l, const Transform& t, std::uint64_t order, core::DrawItem item) {
  item.parallax = l.layer.parallax;
  out.emplace_back(l.layer.z, t.z, order, std::move(item));
}

void particle_items(const entt::registry& registry, float blend, std::vector<Keyed>& out) {
  const auto view = registry.view<const Transform, const Sequence, const Layered, const Emission>(entt::exclude<Doomed>);
  for (auto [e, t, seq, l, em] : view.each()) {
    if (em.emitter.count() == 0) continue;
    core::DrawItem item{em.emitter.spec().look, t.pos, 0.0f};
    em.emitter.quads(blend, item.particles);
    add(out, l, t, seq.value * 2 + 1, std::move(item));
  }
}

}  // namespace

void step_emitters(entt::registry& registry, float dt, core::Random& random) {
  int budget = core::kMaxParticles - particle_count(registry);
  std::vector<entt::entity> finished;
  for (auto [e, t, em] : registry.view<const Transform, Emission>(entt::exclude<Doomed>).each()) {
    budget -= em.emitter.step(dt, t.pos, random, std::max(budget, 0));
    if (em.emitter.finished()) finished.push_back(e);
  }
  for (const entt::entity e : finished) registry.emplace_or_replace<Doomed>(e);
}

int particle_count(const entt::registry& registry) {
  int n = 0;
  for (auto [e, em] : registry.view<const Emission>(entt::exclude<Doomed>).each()) n += em.emitter.count();
  return n;
}

std::vector<core::DrawItem> draw_list(const entt::registry& registry, float blend) {
  std::vector<Keyed> keyed;
  const auto view =
      registry.view<const Transform, const Previous, const Visual, const Sequence, const Layered>(entt::exclude<Doomed>);
  for (auto [e, t, p, v, seq, l] : view.each()) add(keyed, l, t, seq.value * 2, blended(v.shape, p, t, blend));
  particle_items(registry, blend, keyed);
  std::sort(keyed.begin(), keyed.end(), [](const Keyed& a, const Keyed& b) {
    return std::tie(std::get<0>(a), std::get<1>(a), std::get<2>(a)) < std::tie(std::get<0>(b), std::get<1>(b), std::get<2>(b));
  });
  std::vector<core::DrawItem> items;
  items.reserve(keyed.size());
  for (auto& k : keyed) items.push_back(std::move(std::get<3>(k)));
  return items;
}

std::vector<core::LightItem> light_list(const entt::registry& registry, float blend) {
  std::vector<std::pair<std::uint64_t, core::LightItem>> keyed;
  const auto view = registry.view<const Transform, const Previous, const Sequence, const Layered, const Lit>(entt::exclude<Doomed>);
  for (auto [e, t, p, seq, l, lit] : view.each()) {
    keyed.emplace_back(seq.value, core::LightItem{p.pos + (t.pos - p.pos) * blend, lit.light, l.layer.parallax});
  }
  std::sort(keyed.begin(), keyed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
  std::vector<core::LightItem> out;
  for (auto& k : keyed) out.push_back(k.second);
  return out;
}

void rescale(core::ShapeSpec& shape, Scaled& base, float scale) {
  if (!(scale > 0.0f) || !std::isfinite(scale)) {
    throw core::Error("entity.scale must be a number > 0, got " + std::to_string(scale));
  }
  base.scale = scale;
  if (const auto* c = std::get_if<core::Circle>(&base.base)) shape.geometry = core::Circle{c->radius * scale};
  else shape.geometry = core::Rect{std::get<core::Rect>(base.base).size * scale};
  if (shape.text) shape.text->size = base.text_size * scale;
}

}  // namespace alpha::ecs
