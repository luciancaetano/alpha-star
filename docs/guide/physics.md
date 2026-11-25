# Physics

Box2D v3 under a small API. Units are pixels, degrees and kg (50 px = 1 m inside).

```ts
config: { gravity: [0, 900], collision: { categories: ["ship", "enemy", "shot"] } }

body: { type: "dynamic", fixedRotation: true, gravityScale: 1, bullet: false,
        category: "ship", collidesWith: ["enemy"],
        collider: { rect: [12, 16], friction: 0.6, restitution: 0, density: 1 } }
// static (walls), kinematic (moved by velocity only: lifts, enemies on paths), dynamic
// collider: { circle: r } or { rect: [w, h] }; sensor: true (overlaps only); oneWay: true (jump up through)
```

## Contacts

```ts
ctx.onHit("hero", "enemy", (h, e, contact) => { /* contact.normal h -> e, point, speed */ });
ctx.onHitEnd("hero", "ground", (h, g) => {});
ctx.onEnter("player", "coin", (p, c) => {});   // a sensor overlap starts
ctx.onExit("player", "zone", (p, z) => {});
```

Several contacts can arrive in one step: check `e.alive` before acting on an entity a previous
handler may have destroyed. Categories need both sides to accept each other.

## Queries

```ts
ctx.raycast([x, y], [x, y + 12], { tags: ["walls"] });   // { entity, point, normal, fraction } | null
ctx.raycastAll(from, to, { categories: ["enemy"] });
ctx.queryCircle(center, radius, { tags: ["enemy"] });    // sensors included
ctx.queryRect([x, y, w, h]);
```

Raycasts ignore sensors; area queries include them.

## Bodies and joints

```ts
hero.body.velocity = [vx, vy];
hero.body.applyImpulse([0, -hero.body.mass * 300]);   // changes velocity by 300 px/s
const rope = ctx.joint({ type: "rope", a: hook, b: crate, length: 80 });   // revolute | distance | weld | prismatic
rope.destroy();
```

## Tweens and vectors

```ts
import { vec } from "alpha";   // add sub scale len norm dot lerp angle fromAngle dist
ctx.tween(lift, { to: { x: 470 }, seconds: 2.5, ease: "sineInOut", yoyo: true, repeat: Infinity });
ctx.tween(coin, { to: { scale: 2.5, alpha: 0 }, seconds: 0.25, onDone: () => ctx.destroy(coin) });
```

A tweened kinematic body moves by velocity, so riders are carried (`examples/platformer` lift).
