# Entities

> summary: EntitySpec fields, spawn/find/destroy, prefabs as plain functions, tags, parent/child, entity data, looks and bulk positions; read when creating or changing game objects.

## API

```ts
interface EntitySpec {           // plain data; build with functions, never classes
  tags: T[]; pos: Vec2;
  shape?: Shape;                 // circle | rect | image | sheet | text
  body?: BodySpec;               // physics.md
  emitter?: EmitterSpec;         // particles-and-effects.md
  light?: LightSpec;             // particles-and-effects.md
  rotation?: number; z?: number; // z = order inside its layer (default 0)
  layer?: L;                     // a config.layers name
  parent?: Entity;               // pos/rotation local to parent; follows it; destroyed with it
  data?: EntityData;             // JSON-safe plain object, copied
}
ctx.spawn(spec): Entity
ctx.destroy(e)                   // removed at end of step; e.alive false at once; twice = no-op
ctx.find(tag): Entity            // throws if none has the tag
ctx.findAll(tag): Entity[]
ctx.positions(tag): Positions    // { count, ids, x, y: Float64Array; write() }
```

Entity: `id, tags, pos (Vec2Ref {x, y}), rotation, z, body, alive, animation, play(name), flipX, flipY, tint, alpha, blend, text, layer, emit(n), emitting, particles, scale, parent, children, data`.

Filled shapes: `{ circle: r, color, outline?, smooth? }`, `{ rect: [w, h], color, outline?, smooth? }`. Every shape also takes `tint?, alpha?, blend?: "alpha" | "add" | "multiply"`. Colors: `"#rrggbb"` or `"#rrggbbaa"`.

## Prefab (examples/platformer/src/prefabs/coin.ts)

```ts
import type { EntitySpec, LevelEntity } from "alpha";
import type { Platformer } from "../types";
type Spec = EntitySpec<Platformer["tags"], never, Platformer["sheets"]>;

export function coin(at: LevelEntity): Spec {
  return {
    tags: ["coin"], pos: at.pos, z: 1,
    shape: { sheet: "coin", animation: "spin" },
    body: { type: "static", collider: { rect: [8, 8], sensor: true } },
  };
}
```
`EntitySpec` generics: `<Tags, Images, Sheets, Layers, Fonts, Categories>`; `never` for unused slots.

## Usage

```ts
const hero = ctx.spawn(player([40, 100]));
const sword = ctx.spawn({ tags: ["sword"], pos: [8, 0], parent: hero, shape: { rect: [10, 2], color: "#cccccc" } });
const e = ctx.spawn({ ...enemy(at), data: { hp: 3 } });
e.data.hp -= 1;                                      // change in place or replace e.data
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
