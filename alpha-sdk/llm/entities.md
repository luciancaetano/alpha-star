# Entities

> summary: Tags as typed components (GameTypes.tags: tag -> data shape), EntitySpec<G, T> and Entity<G, T>, spawn/find/destroy, has() narrowing, prefabs as plain functions, parent/child, typed entity data, ctx.frozen, looks and bulk positions; read when creating or changing game objects.

## API

```ts
// types.ts: a tag is a typed component, tag -> the data its entities carry ({} = none)
tags: { hero: {}; enemy: { kind: "moth" | "ghoul"; hp: number }; boss: { phase: number } };

type EntitySpec<G, T> = {        // T = the tags this spec uses; plain data; build with functions, never classes
  tags: T[]; pos: Vec2;
  shape?: Shape;                 // circle | rect | image | sheet | text
  body?: BodySpec;               // physics.md
  emitter?: EmitterSpec;         // particles-and-effects.md
  light?: LightSpec;             // particles-and-effects.md
  rotation?: number; z?: number; // z = order inside its layer (default 0)
  layer?: L;                     // a config.layers name
  parent?: Entity;               // pos/rotation local to parent; follows it; destroyed with it
  data: DataOf<G, T>;            // every field of every tag in T; optional only when they have none; JSON-safe, copied
}
ctx.spawn(spec): Entity<G, T>    // a spec missing a tag's data field is a compile error
ctx.destroy(e)                   // e.alive false at once, gone from queries; freed at end of step; twice = no-op
ctx.find(tag): Entity<G, tag>    // oldest living; throws if none has the tag
ctx.findAll(tag): Entity<G, tag>[]   // living only, oldest first
ctx.frozen = true                // freezes the world (physics, timers, tweens, anims, particles, ctx.time); update/UI/input run
ctx.positions(tag): Positions    // { count, ids, x, y: Float64Array; write() }
```

Moves are drawn smoothly between fixed steps; setting `pos` more than a quarter of the view away is a teleport (drawn there at once, children too).

Entity: `id, tags, has(tag), pos (Vec2Ref {x, y}), rotation, z, body, alive, animation, play(name), flipX, flipY, tint, alpha, blend, text, layer, emit(n), emitting, particles, scale, parent, children, data`.

`Entity<G, "enemy">`: `data` is enemy's shape (no casts). `Entity<G>`: tags unknown, `data` is `{}`; every
`Entity<G, T>` is an `Entity<G>`, but a gem is not an `Entity<G, "enemy">` (compile error). `e.has("boss")`
narrows: inside `if (e.has("boss"))` the entity also has boss's data. Queries filtered by `tags` and
`onHit("shot", "enemy", (s, e) => ...)` give typed entities too.

Filled shapes: `{ circle: r, color, outline?, smooth? }`, `{ rect: [w, h], color, outline?, smooth? }`. Every shape also takes `tint?, alpha?, blend?: "alpha" | "add" | "multiply"`. Colors: `"#rrggbb"` or `"#rrggbbaa"`.

## Prefab (examples/platformer/src/prefabs/coin.ts)

```ts
import type { EntitySpec, LevelEntity } from "alpha";
import type { Platformer } from "../types";
export function coin(at: LevelEntity): EntitySpec<Platformer, "coin"> {
  return {
    tags: ["coin"], pos: at.pos, z: 1,
    shape: { sheet: "coin", animation: "spin" },
    body: { type: "static", collider: { rect: [8, 8], sensor: true } },
  };
}
```
Images, sheets, layers, fonts and collision categories are checked from the game type itself. Local aliases help:
`type Spec<T extends TagOf<G>> = EntitySpec<G, T>; type Ent<T extends TagOf<G> = never> = Entity<G, T>;`

## Usage

```ts
const hero = ctx.spawn(player([40, 100]));
const sword = ctx.spawn({ tags: ["sword"], pos: [8, 0], parent: hero, shape: { rect: [10, 2], color: "#cccccc" } });
const e = ctx.spawn({ tags: ["enemy"], pos: at, data: { kind: "moth", hp: 3 } });
e.data.hp -= 1;                                      // a number: change in place or replace e.data
if (e.has("boss")) e.data.phase += 1;                // narrowed to boss data
if (c.data.taken) return; c.data.taken = true;       // guard double handling
for (const t of ctx.findAll("popup")) { t.pos.y -= 20 * ctx.dt; t.alpha = Math.max(0, t.alpha - ctx.dt); }
e.tint = "#ff000080"; e.alpha = 0.5; e.blend = "add"; e.scale = 2; e.flipX = true;
hero.children; sword.parent;

const stars = ctx.positions("star");                 // many entities per frame
for (let i = 0; i < stars.count; i++) stars.y[i] += 40 * ctx.dt;
stars.write();
```

## Rules and gotchas

- `entity.body` throws (pausing the game) without `body`; `entity.text` throws for non-text; `tint` throws with neither shape nor emitter; `emit` throws without `emitter`.
- `ctx.find` throws `find("x"): no living entity has this tag`; use `findAll(tag)[0]` when absence is normal.
- `data` must stay JSON-safe (numbers, strings, booleans, null, arrays, plain objects) or it errors. MCP `game_state` and `run.entities(tag)` show it.
- Stale references are safe to check: test `e.alive` before acting (several contacts can arrive in one step).
- `scale` must be > 0. Tags must be declared in `GameTypes.tags`.
- Entities are scene-scoped; on hot reload `enter` re-creates them (data included).
- Thousands of moving entities: `ctx.positions(tag)` (oldest first; destroyed ones skipped), not `entity.pos` per entity.
