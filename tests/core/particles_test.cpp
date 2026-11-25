#include <doctest/doctest.h>

#include <chrono>
#include <cstdio>

#include "core/particles.hpp"

using alpha::core::Color;
using alpha::core::Emitter;
using alpha::core::EmitterSpec;
using alpha::core::ParticleQuad;
using alpha::core::Random;
using alpha::core::Vec2;

namespace {

constexpr float kDt = 1.0f / 60.0f;

EmitterSpec spray() {
  EmitterSpec s;
  s.lifetime = {0.5f, 0.5f};
  s.speed = {60.0f, 60.0f};
  s.angle = {0.0f, 0.0f};  // to the right
  s.max = 1000;
  return s;
}

std::vector<ParticleQuad> quads_of(const Emitter& e, float blend = 1.0f) {
  std::vector<ParticleQuad> out;
  e.quads(blend, out);
  return out;
}

}  // namespace

TEST_CASE("an emitter emits rate * dt per step, carrying fractions") {
  EmitterSpec s = spray();
  s.rate = 30.0f;  // half a particle per step
  s.lifetime = {10.0f, 10.0f};
  Emitter e(s);
  Random r(1);
  int born = 0;
  for (int i = 0; i < 60; ++i) born += e.step(kDt, {}, r, 1000);
  CHECK(born == 30);
  CHECK(e.count() == 30);
}

TEST_CASE("a burst goes out on the first step; particles fly and die") {
  EmitterSpec s = spray();
  s.burst = 5;
  Emitter e(s);
  Random r(1);
  CHECK(e.step(kDt, {100, 50}, r, 1000) == 5);
  for (int i = 0; i < 29; ++i) e.step(kDt, {100, 50}, r, 1000);
  const auto q = quads_of(e);
  REQUIRE(q.size() == 5);
  CHECK(q[0].pos.x > 120.0f);  // 60 px/s to the right for ~0.5 s
  CHECK(q[0].pos.y == doctest::Approx(50.0f));
  e.step(kDt, {}, r, 1000);
  CHECK(e.count() == 0);
}

TEST_CASE("gravity bends particles; size and color go start -> end") {
  EmitterSpec s = spray();
  s.burst = 1;
  s.gravity = {0, 600};
  s.size = {10, 0};
  s.color_start = {255, 0, 0, 255};
  s.color_end = {0, 0, 255, 0};
  s.lifetime = {1, 1};
  Emitter e(s);
  Random r(1);
  for (int i = 0; i < 30; ++i) e.step(kDt, {}, r, 1000);
  const auto q = quads_of(e);
  REQUIRE(q.size() == 1);
  CHECK(q[0].pos.y > 50.0f);
  CHECK(q[0].size == doctest::Approx(5.0f).epsilon(0.05));
  CHECK(q[0].color.r < 140);
  CHECK(q[0].color.b > 115);
  CHECK(q[0].color.a < 140);
}

TEST_CASE("the budget and spec.max cap emission") {
  EmitterSpec s = spray();
  s.burst = 50;
  s.max = 20;
  Emitter capped(s);
  Random r(1);
  CHECK(capped.step(kDt, {}, r, 1000) == 20);
  Emitter budget(spray());
  budget.burst(50);
  CHECK(budget.step(kDt, {}, r, 7) == 7);
}

TEST_CASE("one-shot emitters finish when their burst died; rate stops when not emitting") {
  EmitterSpec s = spray();
  s.burst = 3;
  s.rate = 600;
  s.one_shot = true;
  Emitter e(s);
  Random r(1);
  CHECK(e.step(kDt, {}, r, 1000) == 3);  // one-shot: the burst only
  CHECK_FALSE(e.finished());
  for (int i = 0; i < 31; ++i) CHECK(e.step(kDt, {}, r, 1000) == 0);
  CHECK(e.finished());
  Emitter off(spray());
  off.spec().rate = 600;
  off.set_emitting(false);
  CHECK(off.step(kDt, {}, r, 1000) == 0);
  off.burst(2);
  CHECK(off.step(kDt, {}, r, 1000) == 2);  // explicit bursts still go out
}

TEST_CASE("same seed, same particles; render interpolation blends prev -> pos") {
  EmitterSpec s = spray();
  s.burst = 4;
  s.angle = {0, 360};
  s.speed = {10, 100};
  Emitter a(s);
  Emitter b(s);
  Random ra(7);
  Random rb(7);
  for (int i = 0; i < 10; ++i) {
    a.step(kDt, {}, ra, 1000);
    b.step(kDt, {}, rb, 1000);
  }
  const auto qa = quads_of(a);
  const auto qb = quads_of(b);
  REQUIRE(qa.size() == qb.size());
  for (std::size_t i = 0; i < qa.size(); ++i) CHECK(qa[i].pos == qb[i].pos);
  const auto half = quads_of(a, 0.0f);
  CHECK(half[0].pos != qa[0].pos);
}

TEST_CASE("shade multiplies by tint and alpha; mix blends") {
  CHECK(alpha::core::shade({200, 100, 50, 255}, {255, 0, 255, 255}, 0.5f) == Color{200, 0, 50, 128});
  CHECK(alpha::core::mix({0, 0, 0, 0}, {200, 100, 50, 255}, 0.5f) == Color{100, 50, 25, 128});
}

TEST_CASE("particle throughput" * doctest::skip()) {
  EmitterSpec s = spray();
  s.max = alpha::core::kMaxParticles;
  s.rate = 1e6f;
  s.lifetime = {1e3f, 1e3f};
  s.gravity = {0, 100};
  Emitter e(s);
  Random r(1);
  e.step(kDt, {}, r, alpha::core::kMaxParticles);
  std::vector<ParticleQuad> out;
  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < 600; ++i) {
    e.step(kDt, {}, r, 0);
    out.clear();
    e.quads(0.5f, out);
  }
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  std::printf("particles: %d live, %.3f ms per step + quads\n", e.count(), ms / 600.0);
}

TEST_CASE("reduced motion scales emitter counts down but keeps bursts and one-shots valid") {
  EmitterSpec s;
  s.rate = 40.0f;
  s.burst = 3;
  s.max = 100;
  s.one_shot = true;
  const EmitterSpec r = alpha::core::reduced_motion(s);
  CHECK(r.rate == doctest::Approx(40.0f * alpha::core::kReducedMotion));
  CHECK(r.burst == 1);
  CHECK(r.max == 25);
  CHECK(r.one_shot);
  EmitterSpec none;
  CHECK(alpha::core::reduced_motion(none).burst == 0);
  CHECK(alpha::core::reduced_count(8) == 2);
  CHECK(alpha::core::reduced_count(1) == 1);
  CHECK(alpha::core::reduced_count(0) == 0);
}
