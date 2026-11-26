# RFC: script API 1.0 (breaking)

Status: phases 1-4 implemented.
Evidence: `examples/survivors` (Candlewick), the largest alpha-star game, and the "lessons" in its AGENTS.md.

## Why

alpha-star's edge is that every name a game uses is typed (`GameTypes`): a misspelled tag, action or
scene is a compile error. That edge stops at the entity: game data, the thing agents touch most, is
`{ [k: string]: SaveValue }`, so Candlewick reads `(e.data.hp as number) - dmg` dozens of times, and
the three most repeated "lessons" in its AGENTS.md are engine gaps, not game lessons:

| Lesson in AGENTS.md | Root cause in the API |
|---|---|
| "Queries can return foes killed this step: always `.filter((e) => e.alive)`" | physics queries return entities destroyed earlier in the step |
| "There is no world pause: keep a `mode` and zero the velocities of moving bodies each step" | no way to freeze the simulation while a menu is up |
| "Prefer your own timers in ctx.state; ctx.every/ctx.after keep running during your menus" | same |
| `e.data.kind as EnemyKind`, `e.data.hp as number` | `entity.data` is untyped |

## What the praised APIs teach (and what we take)

| API | Praised for | Taken here |
|---|---|---|
| LÖVE | small API that covers every tier of complexity; willing to break for a better API | keep `update(ctx)` as the simple path, add structure only where it removes code |
| PICO-8 | tiny, memorizable surface | one way to do each thing; cut duplicates (phase 4) |
| Defold | data-centric, message passing, small public surface | typed tag data; typed game events (phase 2) |
| Bevy | components are typed data; queries are typed; states + run conditions (`run_if(in_state)`) | tag = component with typed data; typed queries; `frozen` now, states/systems (phase 2) |
| Kaplay/Kaboom | composition: an entity's type grows with what it has | `Entity<G, "enemy">`, `e.has("boss")` narrows |
| Godot | signals; `process_mode` makes pause an engine concept | `ctx.frozen` |

## Phase 1 (0.6.0, done): typed tags, alive-only queries, frozen world

### 1. A tag is a component with typed data

```ts
export interface MyGame {
  tags: {
    hero: {};
    enemy: { kind: EnemyKind; hp: number; flash: number };
    boss: {};
    gem: { v: number };
  };
  // ...
}
export type Ent<T extends Tag = never> = Entity<MyGame, T>;   // optional local aliases
type Tag = TagOf<MyGame>;
```

- `tags` was a union of names; it is now `tag -> data shape` (`{}` = no data).
- `ctx.spawn({ tags: ["enemy", "boss"], data: {...} })` requires `data` to hold every field of every
  tag it lists (the intersection), and no `data` when all its tags have `{}`.
- `ctx.find("enemy")`, `findAll`, `queryCircle(.., { tags: ["enemy"] })`, `onHit("shot", "enemy", (s, e) => ..)`
  return `Entity<G, "enemy">`: `e.data.hp` is a `number`, no casts.
- `e.has("boss")` is a type guard: inside `if (e.has("boss"))` the entity also has boss's data.
- Every type is parameterized by the game (`G`) alone: `EntitySpec<G, T>`, `Entity<G, T>`,
  `RayHit<G>`, `JointSpec<G>`, `Level<G>`; no more six-generic `EntitySpec<Tags, Images, Sheets, ...>`.
- `Entity<G>` (no tag) is "some entity": data `{}`; every `Entity<G, T>` is assignable to it.

### 2. Queries only return living entities

`find`, `findAll`, `queryCircle`, `queryRect`, `raycast`, `raycastAll` skip entities destroyed
earlier in the same step. `.filter((e) => e.alive)` after a query is no longer needed (`alive` still
matters for references held across steps and inside loops that kill).

### 3. `ctx.frozen`: pause the world, keep the menu running

`ctx.frozen = true` stops the simulation of the scene: physics (bodies keep their velocity and resume
with it), `ctx.every`/`ctx.after` timers, tweens, animations, particles, animated tiles and contact
events. `update`, UI handlers, input and audio keep running, so a level-up or pause menu needs no
`mode` checks in the world systems, no zeroed velocities and no hand-made timers. `ctx.time` stops too
(it is the scene's simulated time). Reset to false on every scene enter. Different from `ctx.app.paused`
(the OS paused the app: nothing runs at all).

## Phase 2 (done): systems, typed events, timer handles

```ts
export default scene("play", {
  state: () => ({ ... }),
  systems: [
    { name: "chase", tag: "enemy", each(ctx, e) { ... } },           // e: Entity<G, "enemy">
    { name: "menu", when: "frozen", run(ctx) { ... } },
    (ctx) => { ... },
  ],
});
```

- Plain objects, no builder: `{ name?, when?, run(ctx) }` or `{ name?, when?, tag, each(ctx, e) }`.
  The keys differ (`run` vs `each`) on purpose: TypeScript cannot type a parameter from a union
  discriminated by a missing property, so one shape per key keeps `ctx` and `e` inferred.
  Run in array order after `update` (no `after:` graph: order is the array). `tag` systems skip
  entities destroyed earlier in the same pass. `when: "running" | "frozen" | "always"`.
- Typed game events in `GameTypes.events` (`{ died: { enemy: Entity<G, "enemy"> } }`): `ctx.emit("died", {...})`,
  `ctx.on("died", fn)`; delivered at the end of the step after contacts, in order, at most 1000 per step.
- `ctx.every`/`ctx.after` return `Timer { active, cancel() }`; `{ owner: e }` ends one with its entity
  (tweens on an entity already ended with it).

## Phase 3 (done): UI beyond text

`ctx.ui.toggle(doc, id, cls, on)` (`on` required: no hidden toggle state, the condition is the
argument) and `ctx.ui.attr(doc, id, name, value)` (text, number, `true` present, `false`/`null`
removed; an `<img src>`, a `<progress value>`, `disabled`), both with ids typed by `GameTypes.ui`
and checked in tests by `expect.classed` / `expect.attr`. Dropped from the proposal: `ctx.ui.list`
(cloned elements would need ids the type system cannot know; a document with N declared slots,
as Candlewick's level-up cards, stays typed).

## Phase 4 (done): one shape for each thing

- **Vectors:** `entity.pos`, `body.velocity` and `camera.pos` are all live `{x, y}` (reading and
  writing an axis goes to the engine) and all take `[x, y]` assignment. Before, `body.velocity`
  returned a copy (`e.body.velocity.x = 0` silently did nothing) and `e.pos = [x, y]` replaced the
  live object with an array. Every other vector stays a `Vec2` tuple (spawn specs, queries, vec helpers).
- **`Rect`** = `[x, y, width, height]` for camera.view / bounds, queryRect and safeArea.
- **`Color`** for `light.color`, `postfx` tint and `config.lighting.ambient` (was `string`).
- **`ctx.postfx.set`** adds or replaces (keeping its place); `add` is gone.
- **`ctx.audio.music.play / crossfade / stop / playing`** replace `music`, `crossfade`, `stopMusic`, `musicPlaying`.
- Dropped: one contact API (`ctx.on` already means game events; `onHit`/`onEnter`/... are clear and
  typed); entity facets typed by spawn shape and animations per sheet (a tag does not decide the
  sheet or shape, so the types could not be honest).

## Migration of a game (phase 1)

1. `types.ts`: `tags: "a" | "b"` -> `tags: { a: {}; b: { hp: number } }` (the fields your code keeps in `data`).
2. Replace `EntitySpec<G["tags"], G["images"], ...>` with `EntitySpec<G>` (or `EntitySpec<G, "enemy">`)
   and `Entity<G["tags"], any>` with `Entity<G>` / `Entity<G, "enemy">`.
3. Delete the casts on `e.data.*` and the `.filter((e) => e.alive)` after queries.
4. Menus: set `ctx.frozen` instead of `mode` checks + zeroed velocities + hand-made timers.
