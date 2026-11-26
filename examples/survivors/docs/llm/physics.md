# Physics

> summary: Box2D-backed bodies, colliders, collision categories, contact handlers, raycasts/area queries, forces/impulses and joints; read when anything should move physically, collide or be detected.

Units: pixels, degrees, kg (50 px = 1 m internally). `config.gravity: [0, 900]` (px/s²).

## API

```ts
interface BodySpec {
  type: "static" | "kinematic" | "dynamic";  // kinematic: moved by velocity only (lifts, path enemies)
  collider: ({ circle: r } | { rect: [w, h] }) & { sensor?, friction? /*0.6*/, restitution? /*0*/, density? /*1*/, oneWay? };
  velocity?: Vec2; fixedRotation?: boolean; gravityScale?: number /*1*/; bullet?: boolean;
  category?: C; collidesWith?: C[];          // config.collision.categories (max 63)
}
e.body.velocity            // Vec2Ref; assign [vx, vy]
e.body.angularVelocity     // deg/s, + = clockwise on screen
e.body.mass; e.body.type
e.body.applyForce([fx, fy]); e.body.applyImpulse([ix, iy]); e.body.applyTorque(t)

ctx.onHit(tagA, tagB, (a, b, contact) => {})  // solid contact starts; contact { normal a->b, point, speed }
ctx.onHitEnd(tagA, tagB, (a, b) => {})
ctx.onEnter(tagA, tagB, (a, b) => {})         // sensor overlap starts
ctx.onExit(tagA, tagB, (a, b) => {})

ctx.raycast(from, to, { tags?, categories? }): { entity, point, normal, fraction } | null
ctx.raycastAll(from, to, filter?)             // nearest first
ctx.queryCircle(center, radius, filter?)      // entities, oldest first
ctx.queryRect([x, y, w, h], filter?)

ctx.joint({ type: "revolute" | "distance" | "rope" | "weld" | "prismatic", a, b,
  anchor?, anchors?: [onA, onB], axis? /*[1,0]*/, length?, limits?: [lo, hi],
  motor?: { speed, max }, collide? /*false*/ }): Joint   // joint.alive, joint.destroy()
```

## Example (examples/platformer)

```ts
// prefab
body: { type: "dynamic", fixedRotation: true, collider: { rect: PLAYER.size } }
// ground check: short ray down from the feet
const hit = ctx.raycast([p.pos.x, feet - 1], [p.pos.x, feet + 3], { tags: ["walls", "ledge", "lift"] });
// carried by a kinematic lift
const carried = ground?.body.type === "kinematic" ? ground.body.velocity.x : 0;
p.body.velocity = [carried + move * speed, vy];
// sensor pickup
ctx.onEnter("player", "coin", (_p, c) => pick(ctx, c));
// one-way ledge
body: { type: "static", collider: { rect: [48, 6], oneWay: true } }
```

Categories:
```ts
config: { collision: { categories: ["hero", "enemy", "world"] } }
body: { type: "dynamic", category: "hero", collidesWith: ["world", "enemy"], collider: { circle: 6 } }
```

Impulse to change velocity by dv: `e.body.applyImpulse([0, -e.body.mass * 300])`.

## Rules and gotchas

- Raycasts ignore sensors; area queries include them. Sensors fire onEnter/onExit, not onHit.
- Categories: both bodies must accept each other. Declare names in `GameTypes.categories`.
- Several contacts can arrive in one step: check `e.alive` / a `data` flag before acting.
- `onHit` `contact.speed` is approach speed (0 when sliding into contact).
- `oneWay` colliders are solid only for bodies coming from above.
- Fast projectiles: `bullet: true`. Platformer heroes: `fixedRotation: true`.
- Tweening a kinematic body's x/y moves it by velocity, so riders are carried (tweens-and-timers.md).
- Handlers, joints and queries are scene-scoped. Joints go away with either body.
- Motor `max` near what the job needs; a huge max overpowers limits.
- Static level colliders come from `ctx.spawnLevel(world, level, { collide: { IntGridLayer: "tag" } })` (tilemaps.md).
