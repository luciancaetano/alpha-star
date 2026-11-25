#pragma once

#include <entt/entity/registry.hpp>

#include <vector>

#include "core/entity_spec.hpp"
#include "core/random.hpp"

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

}  // namespace alpha::ecs
