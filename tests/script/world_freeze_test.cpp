#include <doctest/doctest.h>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::game_with;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

void steps(Host& host, int n) {
  for (int i = 0; i < n; ++i) host.step({}, kDt);
}

}  // namespace

TEST_CASE("ctx.frozen stops bodies, timers, tweens and time while update keeps running") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    state: () => ({ ticks: 0, updates: 0, x: 0, t: 0 }),
    enter(ctx) {
      ctx.spawn({ tags: ['ball'], pos: [0, 0], body: { type: 'dynamic', collider: { circle: 4 }, velocity: [60, 0] } });
      ctx.every(0.1, () => { ctx.state.ticks += 1; });
      ctx.tween(ctx.state, { to: { t: 1 }, seconds: 1 });
      ctx.frozen = true;
    },
    update(ctx) {
      ctx.state.updates += 1;
      ctx.state.x = ctx.find('ball').pos.x;
      if (ctx.state.updates === 30) {
        check(ctx.state.x === 0 && ctx.state.ticks === 0 && ctx.state.t === 0 && ctx.time === 0, 'frozen: ' + JSON.stringify(ctx.state) + ' time ' + ctx.time);
        ctx.frozen = false;
      }
      if (ctx.state.updates === 61) {
        check(ctx.state.x > 25 && ctx.state.ticks >= 4 && ctx.state.t > 0.4, 'thawed: ' + JSON.stringify(ctx.state));
        check(near(ctx.find('ball').body.velocity.x, 60, 1e-3), 'kept its velocity');
      }
    },)")));
  steps(host, 62);
  REQUIRE_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("ctx.frozen must be a boolean and is false again on scene enter") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    enter(ctx) {
      check(ctx.frozen === false, 'starts thawed');
      ctx.frozen = 1;
    },)")));
  host.step({}, kDt);
  CHECK(host.paused());
  CHECK(host.error().find("ctx.frozen must be true") != std::string::npos);
}

TEST_CASE("entity.has tells whether it has a tag") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    enter(ctx) {
      const e = ctx.spawn({ tags: ['enemy', 'boss'], pos: [0, 0] });
      check(e.has('boss') && e.has('enemy') && !e.has('gem'), 'has');
    },)")));
  host.step({}, kDt);
  REQUIRE_MESSAGE(!host.paused(), host.error());
}
