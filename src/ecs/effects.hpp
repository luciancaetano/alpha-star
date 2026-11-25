#pragma once

#include <entt/entity/registry.hpp>

#include <vector>

#include "core/entity_spec.hpp"
#include "core/random.hpp"
#include "ecs/components.hpp"

// World helpers over the registry (World owns the registry and calls these).
namespace alpha::ecs {

// One fixed step of every living emitter at its entity's position, sharing
// core::kMaxParticles; one-shot emitters that finished are marked Doomed.
void step_emitters(entt::registry& registry, float dt, core::Random& random);
// Live particles of every living emitter.
int particle_count(const entt::registry& registry);
// Living shapes and particle sets ordered by layer z, entity z, then
// creation (an entity's particles right after its shape), placed at the
// blend previous -> current by `blend` in [0, 1].
std::vector<core::DrawItem> draw_list(const entt::registry& registry, float blend);
// Living lights placed at the same blend, in creation order.
std::vector<core::LightItem> light_list(const entt::registry& registry, float blend);
// The shape at `scale` times its spawned size (geometry and text size).
// Throws for scale <= 0 or not finite.
void rescale(core::ShapeSpec& shape, Scaled& base, float scale);

}  // namespace alpha::ecs
