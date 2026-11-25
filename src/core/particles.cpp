#include "core/particles.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace alpha::core {

namespace {

std::uint8_t channel(float v) { return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 255.0f) + 0.5f); }

float lerp(float a, float b, float t) { return a + (b - a) * t; }

float pick(Span s, Random& random) { return s.max > s.min ? random.range(s.min, s.max) : s.min; }

}  // namespace

int reduced_count(int n) { return static_cast<int>(std::ceil(static_cast<float>(n) * kReducedMotion)); }

EmitterSpec reduced_motion(EmitterSpec spec) {
  spec.rate *= kReducedMotion;
  spec.burst = reduced_count(spec.burst);
  spec.max = reduced_count(spec.max);
  return spec;
}

Color mix(Color a, Color b, float t) {
  return {channel(lerp(a.r, b.r, t)), channel(lerp(a.g, b.g, t)), channel(lerp(a.b, b.b, t)),
          channel(lerp(a.a, b.a, t))};
}

Color shade(Color c, Color tint, float alpha) {
  constexpr float k = 1.0f / 255.0f;
  return {channel(static_cast<float>(c.r * tint.r) * k), channel(static_cast<float>(c.g * tint.g) * k),
          channel(static_cast<float>(c.b * tint.b) * k), channel(static_cast<float>(c.a * tint.a) * k * alpha)};
}

Emitter::Emitter(EmitterSpec spec) : spec_(std::move(spec)) {
  particles_.reserve(static_cast<std::size_t>(std::max(spec_.max, 0)));
  pending_ = spec_.burst;
}

int Emitter::step(float dt, Vec2 origin, Random& random, int budget) {
  age(dt);
  int wanted = pending_;
  pending_ = 0;
  spent_ = true;
  if (emitting_ && !spec_.one_shot) {
    carry_ += spec_.rate * dt;
    const auto whole = static_cast<int>(carry_);
    carry_ -= static_cast<float>(whole);
    wanted += whole;
  }
  return emit(wanted, origin, random, budget);
}

// Swap-and-pop the dead, then integrate the rest (semi-implicit Euler).
void Emitter::age(float dt) {
  for (std::size_t i = 0; i < particles_.size();) {
    Particle& p = particles_[i];
    p.age += dt;
    if (p.age >= p.life) {
      p = particles_.back();
      particles_.pop_back();
      continue;
    }
    p.vel = p.vel + spec_.gravity * dt;
    p.prev = p.pos;
    p.pos = p.pos + p.vel * dt;
    ++i;
  }
}

int Emitter::emit(int n, Vec2 origin, Random& random, int budget) {
  const int room = spec_.max - count();
  const int born = std::max(0, std::min({n, room, budget}));
  constexpr float kRadians = std::numbers::pi_v<float> / 180.0f;
  for (int i = 0; i < born; ++i) {
    const float life = std::max(pick(spec_.lifetime, random), 1e-3f);
    const float speed = pick(spec_.speed, random);
    const float angle = pick(spec_.angle, random) * kRadians;
    particles_.push_back({origin, origin, Vec2{std::cos(angle), std::sin(angle)} * speed, 0.0f, life});
  }
  return born;
}

void Emitter::quads(float blend, std::vector<ParticleQuad>& out) const {
  const ShapeSpec& look = spec_.look;
  for (const Particle& p : particles_) {
    const float t = p.age / p.life;
    const Color c = shade(mix(spec_.color_start, spec_.color_end, t), look.tint, look.alpha);
    out.push_back({p.prev + (p.pos - p.prev) * blend, lerp(spec_.size.min, spec_.size.max, t), c});
  }
}

}  // namespace alpha::core
