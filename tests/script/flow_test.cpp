#include <doctest/doctest.h>

#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::game_with;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

void steps(Host& host, int n) {
  for (int i = 0; i < n; ++i) host.step({}, kDt);
}

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

TEST_CASE("systems run in order after update; tag systems skip what the pass killed; when follows frozen") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    state: () => ({ log: [], seen: 0, menu: 0 }),
    enter(ctx) {
      for (let i = 0; i < 3; i++) ctx.spawn({ tags: ['foe'], pos: [i, 0] });
    },
    update(ctx) { ctx.state.log.push('u'); },
    systems: [
      (ctx) => ctx.state.log.push('a'),
      { name: 'b', run(ctx) { ctx.state.log.push('b'); } },
      { tag: 'foe', each(ctx, e) {
        ctx.state.seen += 1;
        for (const other of ctx.findAll('foe')) if (other.id !== e.id) ctx.destroy(other);
      } },
      { when: 'frozen', run(ctx) { ctx.state.menu += 1; } },
      { when: 'always', run(ctx) {
        if (ctx.time === 0 && !ctx.frozen) {
          check(ctx.state.log.join('') === 'uab' && ctx.state.seen === 1 && ctx.state.menu === 0, JSON.stringify(ctx.state));
          ctx.frozen = true;
        } else if (ctx.frozen) {
          check(ctx.state.log.join('') === 'uabu' && ctx.state.menu === 1, 'frozen pass ' + JSON.stringify(ctx.state));
        }
      } },
    ],)")));
  steps(host, 2);
  REQUIRE_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("an error in a system names it; a bad systems entry is an agent-facing error") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    systems: [{ name: 'chase', run() { throw new Error('boom'); } }],)")));
  host.step({}, kDt);
  CHECK(host.paused());
  CHECK(has(host.error(), "system \"chase\": boom"));

  Host bad(box.save());
  bad.load(box.write("b.js", game_with("", R"(systems: [{ when: 'later', run() {} }],)")));
  bad.step({}, kDt);
  CHECK(bad.paused());
  CHECK(has(bad.error(), "systems[0].when must be"));
}

TEST_CASE("events are delivered at the end of the step, in order, handlers' events too; a loop is an error") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    state: () => ({ got: [] }),
    enter(ctx) {
      ctx.on('hit', (p) => { ctx.state.got.push('hit' + p.n); if (p.n === 1) ctx.emit('done'); });
      ctx.on('done', () => ctx.state.got.push('done'));
    },
    update(ctx) {
      if (ctx.time === 0) {
        ctx.emit('hit', { n: 1 });
        ctx.emit('hit', { n: 2 });
        check(ctx.state.got.length === 0, 'not delivered at once');
      } else {
        check(ctx.state.got.join(',') === 'hit1,hit2,done', ctx.state.got.join(','));
      }
    },)")));
  steps(host, 2);
  REQUIRE_MESSAGE(!host.paused(), host.error());

  Host loop(box.save());
  loop.load(box.write("l.js", game_with("", R"(
    enter(ctx) { ctx.on('ping', () => ctx.emit('ping')); ctx.emit('ping'); },)")));
  loop.step({}, kDt);
  CHECK(loop.paused());
  CHECK(has(loop.error(), "events in one step"));
}

TEST_CASE("timers return a handle: cancel stops them; an owner's death ends them") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    state: () => ({ a: 0, b: 0, c: 0 }),
    enter(ctx) {
      const t = ctx.every(0.05, () => { ctx.state.a += 1; });
      const owner = ctx.spawn({ tags: ['o'], pos: [0, 0] });
      ctx.every(0.05, () => { ctx.state.b += 1; }, { owner });
      const once = ctx.after(0.05, () => { ctx.state.c += 1; });
      ctx.after(0.2, () => {
        check(ctx.state.a >= 3 && ctx.state.b >= 3 && ctx.state.c === 1 && !once.active && t.active, 'running ' + JSON.stringify(ctx.state));
        t.cancel();
        t.cancel();
        ctx.destroy(owner);
        ctx.state.a = 0;
        ctx.state.b = 0;
        check(!t.active, 'cancelled');
      });
    },
    update(ctx) {
      if (ctx.time > 0.5) check(ctx.state.a === 0 && ctx.state.b === 0, 'stopped ' + JSON.stringify(ctx.state));
    },)")));
  steps(host, 40);
  REQUIRE_MESSAGE(!host.paused(), host.error());
}
