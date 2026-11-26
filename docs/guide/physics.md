# Physics

**What you'll build:** a small platformer room: a hero that runs and jumps, a stack of crates it
can push, a one-way ledge it jumps up through, coins it collects (sensors) and a spike that
restarts the room.

Physics is Box2D v3 under a small API: give an entity a `body` and the engine moves it, collides
it and tells you about contacts. Units are pixels, degrees and seconds. Replace `src/main.ts` with
this and run `npm start`:

```ts
import { define, type Context } from "alpha";

interface Phys {
  actions: "jump";
  axes: "moveX";
  sounds: "coin";
  images: never;
  tags: { hero: {}; ground: {}; ledge: {}; crate: {}; coin: {}; spike: {} };
  ui: {};
  scenes: { play: undefined };
}

const { scene, game } = define<Phys>();
type Play = Context<Phys, { coins: number }>;

// Standing on something: a short ray down from the feet hits a solid that is not the hero.
function grounded(ctx: Play): boolean {
  const h = ctx.find("hero");
  return ctx.raycast([h.pos.x, h.pos.y + 7], [h.pos.x, h.pos.y + 12], { tags: ["ground", "ledge", "crate"] }) !== null;
}

const play = scene("play", {
  state: () => ({ coins: 0 }),

  enter(ctx) {
    // Static: never moves. The floor and a one-way ledge you can jump up through.
    ctx.spawn({ tags: ["ground"], pos: [160, 172], shape: { rect: [320, 16], color: "#5f574f" }, body: { type: "static", collider: { rect: [320, 16] } } });
    ctx.spawn({ tags: ["ledge"], pos: [220, 110], shape: { rect: [60, 4], color: "#ab5236" }, body: { type: "static", collider: { rect: [60, 4], oneWay: true } } });
    // Dynamic: gravity, collisions, pushes. The hero never tips over.
    ctx.spawn({ tags: ["hero"], pos: [40, 140], shape: { rect: [12, 16], color: "#29adff" }, body: { type: "dynamic", fixedRotation: true, collider: { rect: [12, 16], friction: 0 } } });
    for (let i = 0; i < 3; i++) {
      ctx.spawn({ tags: ["crate"], pos: [120, 156 - i * 16], shape: { rect: [16, 16], color: "#ffa300" }, body: { type: "dynamic", collider: { rect: [16, 16], density: 0.5 } } });
    }
    // Sensors: overlaps only, nothing bumps into them.
    for (const x of [220, 240]) {
      ctx.spawn({ tags: ["coin"], pos: [x, 96], shape: { circle: 4, color: "#ffec27" }, body: { type: "static", collider: { circle: 4, sensor: true } } });
    }
    ctx.spawn({ tags: ["spike"], pos: [290, 160], shape: { rect: [16, 8], color: "#ff004d" }, body: { type: "static", collider: { rect: [16, 8] } } });

    ctx.onEnter("hero", "coin", (_h, c) => {
      ctx.destroy(c);
      ctx.state.coins += 1;
      ctx.audio.play("coin");
    });
    ctx.onHit("hero", "spike", () => ctx.go("play"));
  },

  update(ctx) {
    const h = ctx.find("hero");
    let vy = h.body.velocity.y;
    if (ctx.input.pressed("jump") && grounded(ctx)) vy = -330;
    h.body.velocity = [ctx.input.axis("moveX") * 110, vy];
  },
});

export default game({
  config: {
    title: "Physics",
    size: [320, 180],
    background: "#1d2b53",
    seed: 1,
    gravity: [0, 900],
    actions: { jump: ["Space", "W", "Up", "GamepadA"] },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"] },
  },
  start: "play",
  scenes: { play },
  assets: { sounds: { coin: { wave: "triangle", freq: 1200, duration: 0.08, volume: 0.3 } } },
});
```

Expected: the hero drops onto the floor; walking right shoves the crates; Space jumps (also off
the crates); the ledge lets you jump up through it and stand on it; its two coins vanish with a
blip; touching the red spike restarts the room.

## Step by step

### Gravity

```ts
config: { gravity: [0, 900] }   // px/s² (+y down). Omit it for none (top-down, space games).
```

### Three body types

```ts
// static: never moves (walls, floors, spikes)
ctx.spawn({ tags: ["wall"], pos: [0, 90], body: { type: "static", collider: { rect: [8, 180] } } });
// kinematic: moves only by its velocity (or a tween); pushes dynamics, ignores forces and gravity
ctx.spawn({ tags: ["lift"], pos: [100, 120], shape: { rect: [40, 6], color: "#c2c3c7" }, body: { type: "kinematic", collider: { rect: [40, 6] }, velocity: [30, 0] } });
// dynamic: gravity, forces, collisions
ctx.spawn({ tags: ["ball"], pos: [160, 20], shape: { circle: 6, color: "#ff77a8" }, body: { type: "dynamic", collider: { circle: 6 } } });
```

The body sits at `pos` (the center). `shape` is only how it looks; `collider` is what collides,
so keep their sizes in step.

### Colliders and materials

```ts
ctx.spawn({ tags: ["ground"], pos: [160, 170], shape: { rect: [320, 20], color: "#c2c3c7" },
  body: { type: "static", collider: { rect: [320, 20], friction: 0.05 } } });          // ice
ctx.spawn({ tags: ["ball"], pos: [100, 20], shape: { circle: 6, color: "#ff77a8" },
  body: { type: "dynamic", collider: { circle: 6, restitution: 0.9, density: 2 } } }); // bouncy, heavy
ctx.spawn({ tags: ["crate"], pos: [140, 20], shape: { rect: [16, 16], color: "#ffa300" },
  body: { type: "dynamic", gravityScale: 0.3, collider: { rect: [16, 16] } } });       // floaty
```

| Collider field | Default | Meaning |
|---|---|---|
| `circle: r` / `rect: [w, h]` | | the shape |
| `friction` | 0.6 | 0 = slides forever |
| `restitution` | 0 | bounciness 0..1 |
| `density` | 1 | kg per m² (50 px = 1 m): heavier pushes harder |
| `sensor` | false | overlaps only (`onEnter` / `onExit`) |
| `oneWay` | false | solid only from above: jump up through it |

| Body field | Meaning |
|---|---|
| `velocity` | starting velocity in px/s |
| `fixedRotation` | never rotates (platformer heroes) |
| `gravityScale` | multiplies `config.gravity` (0 = floats) |
| `bullet` | continuous collision for fast projectiles |
| `category`, `collidesWith` | who collides with whom (below) |

### Moving a body

```ts
const b = ctx.find("ball").body;
b.velocity = [120, -200];             // set directly: the usual way to steer heroes
b.velocity.x;                         // read
b.applyImpulse([0, -b.mass * 300]);   // instant: changes velocity by 300 px/s up
b.applyForce([b.mass * 500, 0]);      // this step: accelerates by 500 px/s² right
b.applyTorque(50);
b.angularVelocity = 180;              // degrees per second, clockwise
```

Setting `entity.pos` teleports a body; prefer velocity for anything that should collide on the way.

### Solid contacts: onHit

```ts
ctx.onHit("hero", "enemy", (hero, enemy, contact) => {
  if (contact.normal[1] > 0.7) {           // normal points hero -> enemy: enemy is below = a stomp
    ctx.destroy(enemy);
    hero.body.velocity.y = -250;                     // velocity is live: set one axis
  } else {
    ctx.go("play");                        // hit from the side: start over
  }
});
ctx.onHitEnd("hero", "ground", () => console.log("left the ground"));
ctx.onHit("crate", "ground", (_c, _g, contact) => {
  if (contact.speed > 300) console.log("crash at", contact.point);
});
```

Register handlers once, in `enter`. The first argument always has the first tag. `contact` has
`normal` (unit, first -> second), `point` and `speed` (approach speed in px/s).

### Sensors: onEnter / onExit

```ts
ctx.spawn({ tags: ["zone"], pos: [200, 150], shape: { rect: [40, 40], color: "#00e43640" },
  body: { type: "static", collider: { rect: [40, 40], sensor: true } } });
ctx.onEnter("hero", "zone", () => { ctx.state.inZone = true; });
ctx.onExit("hero", "zone", () => { ctx.state.inZone = false; });
```

Expected: nothing bumps into the zone; the flag is true while the hero overlaps it. Use sensors for
pickups, triggers, doors, hurtboxes and bullets that should not push.

### One-way platforms

```ts
ctx.spawn({ tags: ["ledge"], pos: [220, 110], shape: { rect: [60, 4], color: "#ab5236" },
  body: { type: "static", collider: { rect: [60, 4], oneWay: true } } });
```

Bodies coming from above land on it; from below or the side they pass through.

### Collision categories

```ts
// types: categories: "player" | "foe" | "shot" | "world";
config: { collision: { categories: ["player", "foe", "shot", "world"] } }
```

```ts
ctx.spawn({ tags: ["hero"], pos: [40, 100], shape: { rect: [12, 16], color: "#29adff" },
  body: { type: "dynamic", category: "player", collidesWith: ["foe", "world"], collider: { rect: [12, 16] } } });
ctx.spawn({ tags: ["enemy"], pos: [80, 100], shape: { rect: [12, 12], color: "#ff004d" },
  body: { type: "dynamic", category: "foe", collidesWith: ["player", "shot", "world"], collider: { rect: [12, 12] } } });
```

Two bodies collide only when **both** accept each other. Without `collidesWith` a body accepts all.
Prefab specs need no extra type arguments: `EntitySpec<MyGame, "enemy">` checks `category` and `collidesWith` against `MyGame["categories"]` automatically.

### Raycasts and area queries

```ts
const h = ctx.find("hero");
const hit = ctx.raycast([h.pos.x, h.pos.y], [h.pos.x + 200, h.pos.y], { tags: ["wall", "enemy"] });
if (hit) console.log(hit.entity.tags, hit.point, hit.normal, hit.fraction);   // nearest solid
for (const r of ctx.raycastAll([0, 90], [320, 90], { categories: ["foe"] })) r.entity.tint = "#ff0000";
const near = ctx.queryCircle([h.pos.x, h.pos.y], 48, { tags: ["enemy"] });       // overlapping a circle
const inBox = ctx.queryRect([100, 50, 60, 40]);                                   // [x, y, w, h], top-left
```

Raycasts ignore sensors and a ray that starts inside a collider does not hit it; area queries
include sensors. Filters take `tags` and/or `categories`.

### Joints

```ts
const pin = ctx.spawn({ tags: ["anchor"], pos: [160, 20], body: { type: "static", collider: { circle: 2 } } });
const bob = ctx.spawn({ tags: ["ball"], pos: [220, 20], shape: { circle: 8, color: "#ff77a8" }, body: { type: "dynamic", collider: { circle: 8 } } });

ctx.joint({ type: "revolute", a: pin, b: bob, anchor: [160, 20] });  // hinge around a world point
ctx.joint({ type: "distance", a: pin, b: bob, length: 60 });         // rigid stick
const rope = ctx.joint({ type: "rope", a: pin, b: bob, length: 80 }); // at most 80 apart, slack allowed
ctx.joint({ type: "weld", a: pin, b: bob });                          // glued
if (rope.alive) rope.destroy();
```

Use one of these per pair (they are alternatives). Limits and motors:

```ts
// A cart on a rail: slides 0..120 px along x, driven at 60 px/s
const rail = ctx.spawn({ tags: ["rail"], pos: [60, 150], body: { type: "static", collider: { rect: [4, 4] } } });
const car = ctx.spawn({ tags: ["car"], pos: [60, 150], shape: { rect: [20, 10], color: "#ffa300" }, body: { type: "dynamic", collider: { rect: [20, 10] } } });
ctx.joint({ type: "prismatic", a: rail, b: car, axis: [1, 0], limits: [0, 120], motor: { speed: 60, max: 1000 } });

// A spinning arm (windmill): 90 degrees per second
const hub = ctx.spawn({ tags: ["anchor"], pos: [250, 120], body: { type: "static", collider: { circle: 2 } } });
const arm = ctx.spawn({ tags: ["wheel"], pos: [250, 120], shape: { rect: [60, 6], color: "#c2c3c7" }, body: { type: "dynamic", collider: { rect: [60, 6] } } });
ctx.joint({ type: "revolute", a: hub, b: arm, motor: { speed: 90, max: 100000 } });
```

Revolute `limits` are degrees (`[-45, 45]`), prismatic ones pixels along `axis`, distance ones
`[min, max]` length. `collide: true` lets the two bodies still touch. A joint goes away with
either body or the scene.

### Moving platforms that carry riders

```ts
const lift = ctx.spawn({ tags: ["lift"], pos: [80, 120], shape: { rect: [48, 6], color: "#c2c3c7" },
  body: { type: "kinematic", collider: { rect: [48, 6], friction: 1 } } });
ctx.tween(lift, { to: { x: 240 }, seconds: 3, ease: "sineInOut", yoyo: true, repeat: Infinity });
```

A tweened kinematic body moves by velocity, so dynamic bodies on it ride along. A hero that sets
its own `velocity` each step should add the platform's: `ground.body.velocity.x` (see
`examples/platformer`).

## Recipes

### Double jump

```ts
// state: () => ({ jumps: 0 }); grounded() as in the full example
const h = ctx.find("hero");
if (grounded(ctx)) ctx.state.jumps = 2;
if (ctx.input.pressed("jump") && ctx.state.jumps > 0) {
  ctx.state.jumps -= 1;
  h.body.velocity.y = -300;
}
```

### Enemy patrol that turns at walls and edges

```ts
// types: tags: { enemy: { dir?: number }; ground: {}; wall: {} }
for (const e of ctx.findAll("enemy")) {
  let dir = e.data.dir ?? 1;
  const ahead = e.pos.x + dir * 10;
  const floor = ctx.raycast([ahead, e.pos.y], [ahead, e.pos.y + 16], { tags: ["ground"] });
  const wall = ctx.raycast([e.pos.x, e.pos.y], [ahead + dir * 2, e.pos.y], { tags: ["wall"] });
  if (!floor || wall) dir = -dir;
  e.data.dir = dir;
  e.body.velocity = [dir * 40, e.body.velocity.y];
}
```

### Bullets

```ts
function bullet(from: Vec2, dir: Vec2): EntitySpec<MyGame, "bullet"> {
  return {
    tags: ["bullet"], pos: from, shape: { circle: 2, color: "#ffec27" },
    body: { type: "dynamic", bullet: true, gravityScale: 0, collider: { circle: 2, sensor: true },
            velocity: vec.scale(vec.norm(dir), 300) },
  };
}
// enter:
ctx.onEnter("bullet", "enemy", (b, e) => { ctx.destroy(b); ctx.destroy(e); });
ctx.onEnter("bullet", "wall", (b) => ctx.destroy(b));
// update:
if (ctx.input.pressed("fire")) ctx.spawn(bullet([h.pos.x, h.pos.y], [h.flipX ? -1 : 1, 0]));
```

A sensor bullet passes through what it hits instead of knocking it; `bullet: true` keeps fast
shots from tunneling.

### Explosion that throws crates

```ts
function explosion(ctx: Play, at: Vec2) {
  for (const e of ctx.queryCircle(at, 64, { tags: ["crate", "enemy"] })) {
    if (e.body.type !== "dynamic") continue;
    const away = vec.norm(vec.sub([e.pos.x, e.pos.y], at));
    e.body.applyImpulse(vec.scale(away, e.body.mass * 400));   // +400 px/s away from the blast
  }
  ctx.camera.shake(4, 0.3);
}
```

### Door trigger and press-to-enter

```ts
ctx.spawn({ tags: ["door"], pos: [300, 150], shape: { rect: [16, 24], color: "#ab5236" },
  body: { type: "static", collider: { rect: [16, 24], sensor: true } } });
ctx.onEnter("hero", "door", () => ctx.go("cave", { from: "east" }));   // walk in

// or: track the overlap with onEnter/onExit (Sensors above), then in update:
if (ctx.input.pressed("use") && ctx.state.inZone) ctx.go("cave", { from: "door" });
```

### Grappling rope

```ts
const h = ctx.find("hero");
const hit = ctx.raycast([h.pos.x, h.pos.y], [h.pos.x + 80, h.pos.y - 120], { tags: ["hook"] });
if (hit && ctx.input.pressed("use")) {
  ctx.joint({ type: "rope", a: hit.entity, b: h, anchors: [hit.point, [h.pos.x, h.pos.y]] });
}
```

### Breakable crates

```ts
ctx.onHit("ball", "crate", (_b, crate, contact) => {
  if (contact.speed > 250 && crate.alive) ctx.destroy(crate);
});
```

## Gotchas

- Several contacts can arrive in one step: check `alive` before acting on an entity a previous
  handler destroyed.
- `entity.body` throws for an entity spawned without `body`.
- `onHit` is for solids, `onEnter` for sensors: a pair with a sensor never fires `onHit`.
- Categories need both sides to accept each other.
- Set the hero's horizontal velocity each step and keep its vertical one, or gravity stops working.
- Dynamic heroes want `fixedRotation: true` and usually `friction: 0` (no sticking to walls).
- Big masses: impulses scale with `body.mass`, so write them as `mass * Δv`.

## See also

- [Input](input.md): coyote time, jump buffering, variable jump height.
- [Sprites, camera, LDtk](tilemaps.md): colliders from LDtk IntGrid layers.
- [Particles and effects](particles.md): tweens, dust, screen shake.
- `examples/platformer` (ground rays, lift, one-way ledges), `examples/topdown` (queries, bombs),
  `examples/shmup` (categories, bullets).
