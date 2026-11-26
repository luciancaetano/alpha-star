# Concepts

**What you'll build:** a complete one-file dodge game (move left and right, rocks fall, the score
counts seconds, a hit ends the run) that shows every core idea: the game, scenes, `ctx`, entities,
tags and the loop.

Make a project with `npx alpha init my-game` (see [Getting started](getting-started.md)), then
replace `src/main.ts` with this and run `npm start`:

```ts
import { define } from "alpha";

// 1. Every name the game uses. A typo anywhere below is a compile error.
interface Dodge {
  actions: "restart";
  axes: "moveX";
  sounds: "hit";
  images: never;
  tags: "player" | "rock" | "label";
  ui: {};
  scenes: { play: undefined; over: { score: number } };
}

// 2. Builders bound to those names.
const { scene, game } = define<Dodge>();

// 3. Scenes: the engine calls enter once, then update 60 times a second.
const play = scene("play", {
  state: () => ({ score: 0 }),

  enter(ctx) {
    ctx.spawn({ tags: ["player"], pos: [160, 160], shape: { rect: [16, 16], color: "#29adff" } });
    ctx.spawn({ tags: ["label"], pos: [160, 12], shape: { text: "0" } });
    ctx.every(0.4, () => {
      const x = ctx.random.range(8, 312);
      ctx.spawn({ tags: ["rock"], pos: [x, -8], shape: { circle: 6, color: "#ff004d" }, data: { speed: ctx.random.range(60, 140) } });
    });
    ctx.every(1, () => {
      ctx.state.score += 1;
      ctx.find("label").text = String(ctx.state.score);
    });
  },

  update(ctx) {
    const p = ctx.find("player");
    p.pos.x = Math.min(312, Math.max(8, p.pos.x + ctx.input.axis("moveX") * 150 * ctx.dt));
    for (const r of ctx.findAll("rock")) {
      r.pos.y += (r.data.speed as number) * ctx.dt;
      if (r.pos.y > 190) ctx.destroy(r);
      else if (Math.abs(r.pos.x - p.pos.x) < 14 && Math.abs(r.pos.y - p.pos.y) < 14) {
        ctx.audio.play("hit");
        ctx.go("over", { score: ctx.state.score });
      }
    }
  },
});

const over = scene("over", {
  enter(ctx, { score }) {
    ctx.spawn({ tags: ["label"], pos: [160, 90], shape: { text: `GAME OVER\nscore ${score}\npress R`, color: "#ffec27" } });
  },
  update(ctx) {
    if (ctx.input.pressed("restart")) ctx.go("play");
  },
});

// 4. The game: config, first scene, every scene, every asset.
export default game({
  config: {
    title: "Dodge",
    size: [320, 180],
    background: "#1d2b53",
    seed: 1,
    actions: { restart: ["R", "Enter", "GamepadStart"] },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"] },
  },
  start: "play",
  scenes: { play, over },
  assets: { sounds: { hit: { wave: "noise", freq: 1, duration: 0.2, volume: 0.4 } } },
});
```

Expected: a blue square at the bottom moves with A/D, the arrows or the stick; red circles fall;
the number at the top counts up each second; touching a rock shows GAME OVER, and R starts again.

| Word | Meaning | In the game above |
|---|---|---|
| Game types | Every name the game uses (actions, tags, scenes, ...) | `interface Dodge` |
| Game | Config, first scene, scenes, assets | `game({ ... })` |
| Scene | One screen or level, with its own lifecycle | `play`, `over` |
| `ctx` | Everything a scene can touch (spawn, input, audio, timers, camera) | `ctx.spawn`, `ctx.every` |
| Entity | One object in the scene, made from a spec | the player, rocks, the label |
| Tag | A label to find entities and pair collisions | `"rock"` |
| Spec | Plain data describing an entity | `{ tags, pos, shape }` |

## Step by step

### The game types

Every action, axis, sound, image, tag, UI document and scene is declared once. Everything else
is checked against it: `ctx.find("rokc")` or `ctx.go("ovr")` fail `npx alpha check`.

```ts
interface MyGame {
  actions: "jump" | "pause" | "restart";   // config.actions must map each one
  axes: "moveX";                            // optional: config.axes
  sounds: "hit";                            // assets.sounds must define each one
  images: never;                            // `never` = no images
  tags: "player" | "enemy" | "rock" | "label";
  ui: {};                                   // `{}` = no UI documents
  scenes: { title: undefined; play: { level: number }; over: { score: number } };
}
const { scene, game } = define<MyGame>();
```

Optional keys add more names: `sheets`, `worlds`, `layers`, `fonts`, `categories`, `music`,
`locales`, `buses`, `shaders` (each page of this guide shows its own).

Bigger games split the same thing into files, as the templates do: `src/types.ts` (the
interface), `src/alpha.ts` (`export const { scene, game } = define<MyGame>()`),
`src/scenes/*.ts` (one `export default scene(...)` each), `src/prefabs/*.ts` (spec functions)
and `src/main.ts` (`export default game({...})`).

### Scenes and their lifecycle

```ts
const play = scene("play", {
  state: (params) => ({ score: 0, lives: 3, level: params.level }), // fresh on every enter
  enter(ctx, { level }) {                                            // once, after state()
    console.log("entering level", level);
  },
  update(ctx) {                                                      // every step (1/60 s)
    if (ctx.state.lives <= 0) ctx.go("over", { score: ctx.state.score });
  },
  exit(ctx) {                                                        // once, before the next scene
    ctx.save.set("lastScore", ctx.state.score);
  },
});

const title = scene("title", {
  update(ctx) {
    if (ctx.input.pressed("jump")) ctx.go("play", { level: 1 });
  },
});
```

Expected: `ctx.go` switches at the end of the step. The new scene starts empty: entities,
handlers, timers, tweens, camera and post effects of the old scene are gone. A scene declared
`undefined` in `scenes` takes no params (`ctx.go("title")`); `start` must be one of those.

### ctx: time, random, timers

```ts
ctx.dt;                                      // the fixed step in seconds (1/60)
ctx.time;                                    // seconds since this scene's enter
ctx.random.range(0, 320);                    // float in [0, 320)
ctx.random.int(1, 6);                        // integer in [1, 6]
ctx.random.pick(["red", "green", "blue"]);
ctx.after(2, () => ctx.go("over", { score: ctx.state.score }));   // once, in 2 s of game time
ctx.every(0.5, () => { ctx.state.score += 1; });                   // every 0.5 s, first at 0.5 s
```

Timers run on game time, so they stop while the app is paused and are dropped on scene change.
`Math.random` is disabled: `ctx.random` is seeded by `config.seed`, so every run is the same.

### Entities

```ts
const e = ctx.spawn({
  tags: ["enemy"],
  pos: [100, 50],                                       // x right, y down, in pixels
  shape: { rect: [12, 12], color: "#ff004d", outline: "#000000" },
  rotation: 45,                                         // degrees, clockwise
  z: 1,                                                 // draws over z 0
  data: { hp: 3, name: "grunt" },                       // your own JSON data
});
e.pos.x += 10;          // move it
e.rotation += 90;
e.scale = 2;            // twice as big
e.alpha = 0.5;
e.data.hp = (e.data.hp as number) - 1;
if ((e.data.hp as number) <= 0) ctx.destroy(e);       // gone at the end of the step
e.alive;                // false right after destroy(); safe on stale references
```

Motion is drawn smoothly between fixed steps (so it stays fluid on 120/144 Hz screens).
Setting `pos` more than a quarter of the view away is a teleport: drawn there at once.

Shapes: `{ circle: r }`, `{ rect: [w, h] }` (both with `color`, optional `outline`), an image,
a sheet sprite ([Sprites](tilemaps.md)) or world text:

```ts
ctx.spawn({ tags: ["solid"], pos: [40, 40], shape: { circle: 10, color: "#00e436" } });
ctx.spawn({ tags: ["solid"], pos: [80, 40], shape: { rect: [30, 10], color: "#ffa300", outline: "#ffffff" } });
ctx.spawn({ tags: ["label"], pos: [160, 20], shape: { text: "Hello", size: 16, color: "#ffec27", align: "center" } });
```

A spec can also carry a `body` ([Physics](physics.md)), an `emitter` and a `layer`
([Particles](particles.md)) or a `light` ([Shaders and lighting](shaders.md)).

### Tags: finding entities

```ts
const player = ctx.find("player");                        // throws if none: a clear error
for (const en of ctx.findAll("enemy")) en.pos.y += 30 * ctx.dt;
ctx.findAll("rock").length;                               // how many
```

An entity can have several tags (`tags: ["enemy", "flying"]`). Collision handlers pair tags too:
`ctx.onHit("player", "enemy", ...)` ([Physics](physics.md)).

### Prefabs: functions that return specs

```ts
import type { EntitySpec, Vec2 } from "alpha";

type Spec = EntitySpec<MyGame["tags"]>;

export function enemy(at: Vec2, hp = 3): Spec {
  return { tags: ["enemy"], pos: at, shape: { rect: [12, 12], color: "#ff004d" }, data: { hp } };
}

for (let i = 0; i < 5; i++) ctx.spawn(enemy([40 + i * 30, 40]));
```

Expected: five red squares in a row. A spec is plain data: building it draws nothing, `ctx.spawn`
does. With images, sheets, layers or fonts, widen the alias:
`EntitySpec<MyGame["tags"], MyGame["images"], MyGame["sheets"], MyGame["layers"]>`.

### Parents and children

```ts
const tank = ctx.spawn({ tags: ["tank"], pos: [100, 100], shape: { rect: [24, 16], color: "#5f574f" } });
const gun = ctx.spawn({ tags: ["turret"], pos: [8, 0], parent: tank, shape: { rect: [14, 4], color: "#c2c3c7" } });
gun.rotation = -30;   // local to the tank
tank.pos.x += 1;      // the gun follows
tank.children;        // [gun]
ctx.destroy(tank);    // the gun goes too
```

### Helper functions take a typed ctx

```ts
import type { Context, Entity } from "alpha";

type Play = Context<MyGame, { score: number }>;           // game types + scene state
type Thing = Entity<MyGame["tags"], any>;

function hurt(ctx: Play, target: Thing) { /* ... */ }
```

### The loop and determinism

The engine owns the loop: input is read, `update` runs, physics, tweens, timers, particles,
animations and the camera advance by exactly `ctx.dt`, then the frame is drawn. Same seed + same
inputs = the same game, which is what makes headless tests reliable:

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

export function rocksFallAndEndTheRun() {
  const run = simulate(game, { seconds: 30 });
  run.expect.noErrors();
  run.expect.scene("over");
}
```

See [Testing](testing.md).

## Recipes

### Countdown timer

```ts
// state: () => ({ left: 60, score: 0 })
ctx.every(1, () => {
  ctx.state.left -= 1;
  ctx.find("label").text = `time ${ctx.state.left}`;
  if (ctx.state.left === 0) ctx.go("over", { score: ctx.state.score });
});
```

### Enemy waves that grow

```ts
let wave = 0;
ctx.every(5, () => {
  wave += 1;
  for (let i = 0; i < wave * 2; i++) ctx.spawn(enemy([ctx.random.range(16, 304), -10]));
});
```

### Health, hit flash and death

```ts
function damage(ctx: Play, target: Thing, amount: number) {
  target.data.hp = (target.data.hp as number) - amount;
  target.alpha = 0.3;                                               // blink...
  ctx.after(0.1, () => { if (target.alive) target.alpha = 1; });    // ...and back
  if ((target.data.hp as number) <= 0) {
    ctx.destroy(target);
    ctx.state.score += 10;
  }
}
```

### Pause

```ts
const play = scene("play", {
  state: () => ({ paused: false }),
  update(ctx) {
    if (ctx.input.pressed("pause")) ctx.state.paused = !ctx.state.paused;
    if (ctx.state.paused) return;
    // ... the rest of the game
  },
});
```

Physics, timers and tweens keep running while `update` returns early; for a full freeze keep
moving things in `update` (or stop their bodies). The app itself pauses on focus loss
(`ctx.onPause`, `config.pauseOnFocusLoss`).

### Game over with a best score

```ts
const over = scene("over", {
  enter(ctx, { score }) {
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    ctx.spawn({ tags: ["label"], pos: [160, 90], shape: { text: `score ${score}\nbest ${best}` } });
  },
  update(ctx) {
    if (ctx.input.pressed("restart")) ctx.go("play", { level: 1 });
  },
});
```

`ctx.go` to the scene you are in restarts it with fresh state. See [Saves](saves.md).

## Gotchas

- Move by `speed * ctx.dt`, never by a fixed amount per frame of the screen.
- `ctx.find` throws when nothing has the tag; use `ctx.findAll(tag)[0]` when it may be missing.
- `ctx.destroy` removes at the end of the step: check `e.alive` in loops and handlers.
- `state()` runs on every enter; values kept across scenes go in params or `ctx.save`.
- `entity.data` must be JSON-safe (numbers, strings, booleans, null, arrays, plain objects).
- Spawning in `update` without a limit grows forever: destroy what leaves the screen.
- Run `npx alpha check` after editing the types: errors name the file and line.

## Later topics

**Pack.** `npx alpha export` type-checks, bundles `src/main.ts` and its imports into
`dist/main.js` (with a source map, so errors point at your TypeScript), packs PNGs into atlas
pages and copies declared assets. See [Exporting](exporting.md).

**Build flavors.** `alpha run` / `alpha test` / `alpha mcp` use the *release* engine (dev tools on
F12, MCP, test runner); `alpha export --standalone` ships the *shipping* engine (none of those).
Nothing in a game can depend on dev features.

**Image filtering.** Each image, sheet or LDtk world picks `filter`: `"nearest"` (crisp),
`"linear"` (smooth), `"sharp"` (pixel art at any size or angle; 16x texture memory). Without it,
`config.pixelMode` decides (`true` = nearest). Shapes and emitters take `smooth`.

`size` is the design canvas for the world and the UI, not the window: raise it and everything
gets smaller on screen. The first window is 2x it (integer scale) or 1.5x, shrunk to fit the screen.

**Window stretch.** `config.window` fits the design `size` to the screen:

```ts
window: { aspect: "expand", scale: "fractional" }  // aspect: expand | keep_width | keep_height | keep; scale: fractional | integer
```

| aspect | screen wider than the design | screen taller than the design |
|---|---|---|
| `expand` (default) | more world left and right | more world above and below |
| `keep_width` | side bars | more world above and below |
| `keep_height` | more world left and right | bars above and below |
| `keep` | side bars | bars above and below |

`ctx.camera.view` is the world rect shown now: size backgrounds to it and spawn at its edges.
HUDs use `ctx.safeArea` (or `class="safe"` in UI). `scale: "integer"` keeps pixel art exact. Other
window options: `fullscreen`, `resizable`, `altEnter`, `borderless`, `alwaysOnTop`, `maximized`,
`vsync`. Test a phone screen with `simulate(game, { seconds: 1, size: [1080, 2340] })`.

## See also

- [Your first game](first-game.md): a guided build.
- [Input](input.md), [Physics](physics.md), [Sprites, camera, LDtk](tilemaps.md), [Particles and effects](particles.md).
- [UI](ui.md), [Text](text.md), [Audio](audio.md), [Saves](saves.md), [Testing](testing.md).
- [API reference](reference.md): every type and function.
