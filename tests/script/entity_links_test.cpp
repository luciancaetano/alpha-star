#include <doctest/doctest.h>

#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::game_with;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

}  // namespace

TEST_CASE("children follow their parent (rotated), go with it, and list each other") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    enter(ctx) {
      const p = ctx.spawn({ tags: ['p'], pos: [100, 100] });
      const c = ctx.spawn({ tags: ['c'], pos: [10, 0], parent: p });
      ctx.spawn({ tags: ['g'], pos: [5, 0], parent: c });
      check(c.pos.x === 110 && c.parent.id === p.id && p.children[0].id === c.id && p.parent === null, 'links');
      p.rotation = 90;
      check(near(c.pos.x, 100, 1e-3) && near(c.pos.y, 110, 1e-3), 'rotated offset ' + c.pos.x + ',' + c.pos.y);
      check(near(ctx.find('g').pos.y, 115, 1e-3), 'grandchild');
      c.pos.x = 120;
      p.pos.x = 200;
      check(near(c.pos.x, 220, 1e-3), 'moving a child changes its offset');
    },
    update(ctx) {
      if (ctx.time > 0) ctx.destroy(ctx.find('p'));
    },)")));
  host.step({}, kDt);
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(host.world()->entities().size() == 3);
  host.step({}, kDt);
  host.step({}, kDt);
  CHECK(host.world()->entities().empty());
}

TEST_CASE("entity data is copied, JSON-checked and mirrored to the engine for tools") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    enter(ctx) {
      const proto = { hp: 3, items: ['key'] };
      const a = ctx.spawn({ tags: ['a'], pos: [0, 0], data: proto });
      a.data.hp -= 1;
      check(proto.hp === 3 && a.data.hp === 2, 'spawn copies the data');
      check(Object.keys(ctx.spawn({ tags: ['b'], pos: [0, 0] }).data).length === 0, 'empty by default');
    },)")));
  host.step({}, kDt);
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(host.world()->data(host.world()->entities()[0]) == R"({"hp":2,"items":["key"]})");

  Host bad(box.save());
  bad.load(box.write("b.js", game_with("", "enter(ctx) { ctx.spawn({ tags: [], pos: [0, 0], data: { f: () => 1 } }); },")));
  CHECK(has(bad.error(), "spawn: data must be a JSON-safe plain object"));
  Host later(box.save());
  later.load(box.write("c.js", game_with("", "update(ctx) { ctx.spawn({ tags: [], pos: [0, 0] }).data.when = new Date(); },")));
  later.step({}, kDt);
  CHECK(has(later.error(), "data must stay JSON-safe"));
}

TEST_CASE("entity.scale resizes the shape from its spawned size") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    enter(ctx) {
      const e = ctx.spawn({ tags: ['e'], pos: [0, 0], shape: { rect: [10, 4], color: '#fff' } });
      e.scale = 2;
      e.scale = 0.5;
      check(e.scale === 0.5, 'reads back');
    },)")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto items = host.world()->draw_items();
  REQUIRE(items.size() == 1);
  const auto size = std::get<alpha::core::Rect>(items[0].shape.geometry).size;
  CHECK(size.x == doctest::Approx(5));
  CHECK(size.y == doctest::Approx(2));
  Host bad(box.save());
  bad.load(box.write("b.js", game_with("", "enter(ctx) { ctx.spawn({ tags: [], pos: [0, 0], shape: { circle: 1, color: '#fff' } }).scale = 0; },")));
  CHECK(has(bad.error(), "entity.scale must be a number > 0"));
}

TEST_CASE("vectors: pos, body.velocity and camera.pos are live {x, y}; assigning [x, y] replaces them") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", R"(
    enter(ctx) {
      const e = ctx.spawn({ tags: ['b'], pos: [10, 20], body: { type: 'dynamic', collider: { circle: 2 }, velocity: [30, 40] } });
      const v = e.body.velocity;
      v.x = 0;
      check(e.body.velocity.x === 0 && near(e.body.velocity.y, 40, 1e-3), 'velocity is live: ' + JSON.stringify([v.x, v.y]));
      e.body.velocity = [5, 6];
      check(v.x === 5 && v.y === 6, 'the same live object reads the new velocity');
      const p = e.pos;
      e.pos = [70, 80];
      check(p.x === 70 && p.y === 80 && e.pos === p, 'pos assigned from [x, y], same live object');
      ctx.camera.pos = [1, 2];
      check(ctx.camera.pos.x === 1 && ctx.camera.pos.y === 2, 'camera.pos assigned');
      ctx.camera.pos.x = 9;
      check(ctx.camera.pos.x === 9, 'camera.pos is live');
    },)")));
  host.step({}, kDt);
  REQUIRE_MESSAGE(!host.paused(), host.error());

  Host bad(box.save());
  bad.load(box.write("b.js", game_with("", "enter(ctx) { ctx.spawn({ tags: ['a'], pos: [0, 0] }).pos = { x: 1, y: 2 }; },")));
  bad.step({}, kDt);
  CHECK(bad.error().find("pos must be [x, y]") != std::string::npos);
}
