#pragma once

#include <vector>

#include "core/entity_spec.hpp"
#include "core/random.hpp"

// Particle simulation (pure). Stepped on the fixed step with the game's
// seeded random stream, so runs and simulate() are reproducible. Each
// emitter owns a pool reserved once (spec.max): no allocation per particle.
namespace alpha::core {

// Most particles alive in one world at once (all emitters together).
inline constexpr int kMaxParticles = 20000;

struct Particle {
  Vec2 pos;
  Vec2 prev;  // pos at the start of the step (render interpolation)
  Vec2 vel;
  float age = 0.0f;
  float life = 1.0f;
};

class Emitter {
 public:
  explicit Emitter(EmitterSpec spec);

  // One fixed step: ages and moves the live particles, then emits the
  // pending burst and `rate * dt` new ones at `origin`, at most `budget`
  // (what the world still allows). Returns how many were born.
  int step(float dt, Vec2 origin, Random& random, int budget);
  // Queues n particles for the next step (entity.emit).
  void burst(int n) { pending_ += n; }
  bool emitting() const { return emitting_; }
  void set_emitting(bool on) { emitting_ = on; }
  int count() const { return static_cast<int>(particles_.size()); }
  // One-shot emitter whose burst went out and whose particles all died.
  bool finished() const { return spec_.one_shot && spent_ && particles_.empty(); }
  EmitterSpec& spec() { return spec_; }
  const EmitterSpec& spec() const { return spec_; }
  // Live particles placed at prev -> pos by `blend`, sized and colored by age.
  void quads(float blend, std::vector<ParticleQuad>& out) const;

 private:
  void age(float dt);
  int emit(int n, Vec2 origin, Random& random, int budget);

  EmitterSpec spec_;
  std::vector<Particle> particles_;
  float carry_ = 0.0f;  // fraction of a particle owed by `rate`
  int pending_ = 0;
  bool emitting_ = true;
  bool spent_ = false;  // the first burst went out
};

// a -> b by t in [0, 1], per channel.
Color mix(Color a, Color b, float t);
// c multiplied by tint, its alpha also by `alpha` (0..1).
Color shade(Color c, Color tint, float alpha);

}  // namespace alpha::core
