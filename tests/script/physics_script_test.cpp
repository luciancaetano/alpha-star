#include <doctest/doctest.h>

#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::game_with;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// Loads the game, runs `n` steps; returns the error ("" when it ran clean).
std::string run(const Sandbox& box, const std::string& config, const std::string& scene, int n) {
  Host host(box.save());
  host.load(box.write("g.js", game_with(config, scene)));
  for (int i = 0; i < n && !host.paused(); ++i) host.step({}, kDt);
  return host.error();
}

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

const std::string kGround = "ctx.spawn({ tags: ['ground'], pos: [200, 250], body: { type: 'static', collider: { rect: [400, 20] } } });";

}  // namespace

TEST_CASE("onHit passes the contact; onHitEnd and onExit report separation") {
  Sandbox box;
  const std::string err = run(box, ", gravity: [0, 900]", R"(
    state: () => ({ hit: null, ended: 0, entered: 0, exited: 0 }),
    enter(ctx) {
      )" + kGround + R"(
      ctx.spawn({ tags: ['ball'], pos: [100, 150], body: { type: 'dynamic', collider: { circle: 10, restitution: 0.9 } } });
      ctx.spawn({ tags: ['zone'], pos: [300, 200], body: { type: 'kinematic', velocity: [-400, 0], collider: { rect: [10, 10], sensor: true } } });
      ctx.spawn({ tags: ['ghost'], pos: [200, 200], body: { type: 'dynamic', gravityScale: 0, collider: { circle: 5 } } });
      ctx.onHit('ball', 'ground', (b, g, c) => { ctx.state.hit ??= c; });
      ctx.onHitEnd('ball', 'ground', () => { ctx.state.ended += 1; });
      ctx.onEnter('ghost', 'zone', () => { ctx.state.entered += 1; });
      ctx.onExit('ghost', 'zone', () => { ctx.state.exited += 1; });
    },
    update(ctx) {
      if (ctx.time < 1.4) return;
      const h = ctx.state.hit;
      check(h && near(h.normal[1], 1, 0.01) && near(h.point[1], 240, 2) && h.speed > 300, 'contact ' + JSON.stringify(h));
      check(ctx.state.ended >= 1, 'bounce ends the contact');
      check(ctx.state.entered === 1 && ctx.state.exited === 1, 'sensor enter + exit');
    },)", 90);
  CHECK_MESSAGE(err.empty(), err);
}

TEST_CASE("raycasts and area queries filter by tags and collision categories") {
  Sandbox box;
  const std::string err = run(box, ", collision: { categories: ['world', 'enemy'] }", R"(
    enter(ctx) {
      const box = (tags, x, category) => ctx.spawn({ tags, pos: [x, 100], body: { type: 'static', category, collider: { rect: [20, 20] } } });
      const wall = box(['wall'], 100, 'world');
      const foe = box(['foe'], 200, 'enemy');
      ctx.spawn({ tags: ['coin'], pos: [50, 100], body: { type: 'static', collider: { rect: [10, 10], sensor: true } } });
      const first = ctx.raycast([0, 100], [300, 100]);
      check(first.entity.id === wall.id && near(first.point[0], 90) && near(first.normal[0], -1, 0.01) && near(first.fraction, 0.3, 0.01), 'first ' + JSON.stringify(first.point));
      check(ctx.raycastAll([0, 100], [300, 100]).length === 2, 'all hits, sensors ignored');
      check(ctx.raycast([0, 100], [300, 100], { tags: ['foe'] }).entity.id === foe.id, 'tag filter');
      check(ctx.raycast([0, 100], [300, 100], { categories: ['enemy'] }).entity.id === foe.id, 'category filter');
      check(ctx.raycast([0, 0], [300, 0]) === null, 'miss');
      check(ctx.queryCircle([72, 100], 20).map((e) => e.tags[0]).join() === 'wall,coin', 'circle, oldest first');
      check(ctx.queryRect([180, 80, 50, 50], { tags: ['foe', 'wall'] })[0].id === foe.id, 'rect');
    },)", 1);
  CHECK_MESSAGE(err.empty(), err);
  CHECK(has(run(box, "", "enter(ctx) { ctx.raycast([0, 0], [1, 1], { categories: ['x'] }); },", 1),
            "categories: \"x\" is not in config.collision.categories; have: (none)"));
  CHECK(has(run(box, ", collision: { categories: ['a'] }",
                "enter(ctx) { ctx.spawn({ tags: [], pos: [0, 0], body: { type: 'static', category: 'b', collider: { circle: 1 } } }); },", 1),
            "spawn: body.category \"b\" is not in config.collision.categories; have: a"));
  CHECK(has(run(box, ", collision: { categories: 'a' }", "", 1), "config.collision must be { categories"));
}

TEST_CASE("collidesWith lets bodies pass through other categories; oneWay holds from above only") {
  Sandbox box;
  const std::string err = run(box, ", gravity: [0, 900], collision: { categories: ['solid', 'ghost'] }", R"(
    enter(ctx) {
      ctx.spawn({ tags: ['floor'], pos: [200, 250], body: { type: 'static', category: 'solid', collider: { rect: [400, 20] } } });
      ctx.spawn({ tags: ['g'], pos: [100, 200], body: { type: 'dynamic', category: 'ghost', collidesWith: ['ghost'], collider: { circle: 5 } } });
      ctx.spawn({ tags: ['ledge'], pos: [300, 150], body: { type: 'static', collider: { rect: [80, 6], oneWay: true } } });
      ctx.spawn({ tags: ['lander'], pos: [300, 100], body: { type: 'dynamic', collider: { circle: 5 } } });
      ctx.spawn({ tags: ['riser'], pos: [320, 240], body: { type: 'dynamic', velocity: [0, -500], collider: { circle: 5 } } });
    },
    update(ctx) {
      if (ctx.time < 0.9) return;
      check(ctx.find('g').pos.y > 300, 'ghost fell through the floor');
      check(near(ctx.find('lander').pos.y, 142, 2), 'lander rests on the ledge: ' + ctx.find('lander').pos.y);
    },)", 60);
  CHECK_MESSAGE(err.empty(), err);
}

TEST_CASE("body extras: fixedRotation, mass, impulses, torque, angular velocity, bullet, materials") {
  Sandbox box;
  const std::string err = run(box, "", R"(
    enter(ctx) {
      const a = ctx.spawn({ tags: ['a'], pos: [100, 100], body: { type: 'dynamic', fixedRotation: true, bullet: true, collider: { circle: 10, density: 2, friction: 0.1 } } });
      const b = ctx.spawn({ tags: ['b'], pos: [200, 100], body: { type: 'dynamic', collider: { circle: 10 } } });
      check(near(a.body.mass, 2 * b.body.mass, 1e-4), 'density doubles mass');
      a.body.applyImpulse([a.body.mass * 120, 0]);
      check(near(a.body.velocity.x, 120, 1e-3), 'impulse / mass = dv');
      b.body.angularVelocity = 90;
      check(near(b.body.angularVelocity, 90, 1e-3), 'angular velocity in degrees/s');
      a.body.applyTorque(1e6);
      b.body.applyForce([0, 600 * b.body.mass]);
      check(a.body.type === 'dynamic', 'type');
    },
    update(ctx) {
      check(ctx.find('a').rotation === 0, 'fixed rotation never turns');
    },)", 10);
  CHECK_MESSAGE(err.empty(), err);
  CHECK(has(run(box, "", "enter(ctx) { ctx.spawn({ tags: [], pos: [0, 0], body: { type: 'static', collider: { circle: 1, friction: -1 } } }); },", 1),
            "must be >= 0"));
  CHECK(has(run(box, "", "enter(ctx) { ctx.spawn({ tags: [], pos: [0, 0] }).body.mass; },", 1), "has no body"));
}

TEST_CASE("joints: revolute holds, rope slacks, destroy frees; bad joints say what exists") {
  Sandbox box;
  const std::string err = run(box, ", gravity: [0, 900]", R"(
    state: () => ({ j: null }),
    enter(ctx) {
      const pin = ctx.spawn({ tags: ['pin'], pos: [100, 50], body: { type: 'static', collider: { circle: 2 } } });
      const bob = ctx.spawn({ tags: ['bob'], pos: [150, 50], body: { type: 'dynamic', collider: { circle: 5 } } });
      ctx.state.j = ctx.joint({ type: 'revolute', a: pin, b: bob, anchor: [100, 50] });
      const top = ctx.spawn({ tags: ['top'], pos: [300, 50], body: { type: 'static', collider: { circle: 2 } } });
      const hang = ctx.spawn({ tags: ['hang'], pos: [300, 80], body: { type: 'dynamic', collider: { circle: 5 } } });
      ctx.joint({ type: 'rope', a: top, b: hang, length: 60 });
    },
    update(ctx) {
      const j = ctx.state.j;
      if (Math.round(ctx.time * 60) === 30) {
        check(j.alive && near(vec.dist([ctx.find('bob').pos.x, ctx.find('bob').pos.y], [100, 50]), 50, 2), 'pinned at 50 px ' + j.alive + ' ' + ctx.find('bob').pos.x + ',' + ctx.find('bob').pos.y);
        check(near(ctx.find('hang').pos.y, 110, 3), 'rope stops at 60 px: ' + ctx.find('hang').pos.y);
        j.destroy();
        check(!j.alive, 'destroyed');
      }
    },)", 40);
  CHECK_MESSAGE(err.empty(), err);
  CHECK(has(run(box, "", "enter(ctx) { ctx.joint({ type: 'spring' }); },", 1), "type must be one of revolute, distance, rope, weld, prismatic"));
  CHECK(has(run(box, "", "enter(ctx) { const e = ctx.spawn({ tags: [], pos: [0, 0] }); ctx.joint({ type: 'weld', a: e, b: ctx.spawn({ tags: [], pos: [1, 0] }) }); },", 1),
            "has no body"));
}

TEST_CASE("joint limits and motors: a prismatic slider stops at its upper limit") {
  Sandbox box;
  const std::string err = run(box, "", R"(
    enter(ctx) {
      const rail = ctx.spawn({ tags: ['rail'], pos: [100, 100], body: { type: 'static', collider: { circle: 2 } } });
      const car = ctx.spawn({ tags: ['car'], pos: [100, 100], body: { type: 'dynamic', collider: { circle: 5 } } });
      ctx.joint({ type: 'prismatic', a: rail, b: car, axis: [1, 0], limits: [0, 60], motor: { speed: 200, max: 1000 } });
      const hub = ctx.spawn({ tags: ['hub'], pos: [300, 100], body: { type: 'static', collider: { circle: 2 } } });
      const arm = ctx.spawn({ tags: ['arm'], pos: [300, 100], body: { type: 'dynamic', collider: { rect: [20, 4] } } });
      ctx.joint({ type: 'revolute', a: hub, b: arm, limits: [-45, 30], motor: { speed: 180, max: 100 } });
    },
    update(ctx) {
      if (Math.round(ctx.time * 60) !== 90) return;
      check(near(ctx.find('car').pos.x, 160, 2), 'slider at its limit: ' + ctx.find('car').pos.x);
      check(near(ctx.find('arm').rotation, 30, 2), 'arm at its upper angle: ' + ctx.find('arm').rotation);
    },)", 95);
  CHECK_MESSAGE(err.empty(), err);
  const std::string two = "const a = ctx.spawn({ tags: [], pos: [0, 0], body: { type: 'static', collider: { circle: 1 } } });"
                          "const b = ctx.spawn({ tags: [], pos: [9, 0], body: { type: 'dynamic', collider: { circle: 1 } } });";
  CHECK(has(run(box, "", "enter(ctx) { " + two + " ctx.joint({ type: 'revolute', a, b, limits: [10, -10] }); },", 1),
            "joint: limits must be [lower, upper] with lower <= upper"));
  CHECK(has(run(box, "", "enter(ctx) { " + two + " ctx.joint({ type: 'revolute', a, b, motor: 5 }); },", 1),
            "joint: motor must be { speed, max }"));
}

TEST_CASE("tweens ease entity and object values on the fixed step; yoyo, repeat, delay, onDone") {
  Sandbox box;
  const std::string err = run(box, "", R"(
    state: () => ({ o: { v: 0 }, done: 0, h: null }),
    enter(ctx) {
      const e = ctx.spawn({ tags: ['e'], pos: [0, 0], shape: { rect: [10, 10], color: '#fff' } });
      ctx.state.h = ctx.tween(e, { to: { x: 60, alpha: 0, scale: 2 }, seconds: 1, onDone: () => { ctx.state.done += 1; } });
      ctx.tween(ctx.state.o, { to: { v: 10 }, seconds: 0.5, ease: 'quadOut', delay: 0.5, yoyo: true, repeat: 1 });
    },
    update(ctx) {
      const e = ctx.find('e');
      const t = Math.round(ctx.time * 60);
      if (t === 30) check(near(e.pos.x, 30, 0.01) && near(e.alpha, 0.5, 0.01) && near(e.scale, 1.5, 0.01), 'linear halfway ' + e.pos.x);
      if (t === 60) check(e.pos.x === 60 && ctx.state.done === 1 && ctx.state.h.done, 'ended once');
      if (t === 60 + 15) check(near(ctx.state.o.v, 7.5, 0.01), 'quadOut at half: ' + ctx.state.o.v);
      if (t === 120) check(near(ctx.state.o.v, 10, 0.01), 'second cycle peak ' + ctx.state.o.v);
      if (t === 200) check(ctx.state.o.v === 0, 'yoyo ends at the start');
    },)", 205);
  CHECK_MESSAGE(err.empty(), err);
  CHECK(has(run(box, "", "enter(ctx) { ctx.tween({ v: 0 }, { to: { v: 1 }, seconds: 1, ease: 'wobbly' }); },", 1), "unknown ease \"wobbly\"; eases: linear, quadIn"));
  CHECK(has(run(box, "", "enter(ctx) { ctx.tween(ctx.spawn({ tags: [], pos: [0, 0] }), { to: { width: 1 }, seconds: 1 }); },", 1),
            "cannot tween \"width\"; entity keys: x, y, rotation, alpha, scale"));
}

TEST_CASE("a tweened kinematic body moves by velocity and carries what stands on it") {
  Sandbox box;
  const std::string err = run(box, ", gravity: [0, 900]", R"(
    enter(ctx) {
      const lift = ctx.spawn({ tags: ['lift'], pos: [100, 200], body: { type: 'kinematic', collider: { rect: [80, 10] } } });
      ctx.spawn({ tags: ['rider'], pos: [100, 180], body: { type: 'dynamic', fixedRotation: true, collider: { rect: [10, 10], friction: 1 } } });
      ctx.after(0.5, () => ctx.tween(lift, { to: { x: 200 }, seconds: 1, ease: 'sineInOut' }));
    },
    update(ctx) {
      const lift = ctx.find('lift');
      if (ctx.time > 1.6) {
        check(near(lift.pos.x, 200, 0.5) && near(lift.body.velocity.x, 0, 0.01), 'lift stopped at 200: ' + lift.pos.x);
        check(near(ctx.find('rider').pos.x, 200, 4), 'rider carried: ' + ctx.find('rider').pos.x);
      }
    },)", 110);
  CHECK_MESSAGE(err.empty(), err);
}

TEST_CASE("vec helpers work on plain arrays") {
  Sandbox box;
  const std::string err = run(box, "", R"(
    enter() {
      check(vec.add([1, 2], [3, 4]).join() === '4,6' && vec.sub([1, 2], [3, 4]).join() === '-2,-2', 'add sub');
      check(vec.scale([1, 2], 3).join() === '3,6' && vec.len([3, 4]) === 5 && vec.dist([0, 0], [3, 4]) === 5, 'scale len dist');
      check(vec.norm([0, 5]).join() === '0,1' && vec.norm([0, 0]).join() === '0,0' && vec.dot([1, 2], [3, 4]) === 11, 'norm dot');
      check(vec.lerp([0, 0], [10, 20], 0.5).join() === '5,10' && near(vec.angle([0, 1]), 90, 1e-9), 'lerp angle');
      const f = vec.fromAngle(180, 2);
      check(near(f[0], -2, 1e-9) && near(f[1], 0, 1e-9), 'fromAngle');
    },)", 1);
  CHECK_MESSAGE(err.empty(), err);
}
