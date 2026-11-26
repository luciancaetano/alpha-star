# Tutorial: Hop, a whole game in one file

Build **Hop**: tap to hop a little square through gaps between moving columns. One file
(`src/main.ts`), shapes only (no images), synthesized sounds (no audio files), a title screen, a
score, a saved best score, sparks, screen shake and automated tests. About 170 lines.

![Hop: a yellow square between two columns](tutorial-hop.png)

**Before you start:** follow [Getting started](getting-started.md) up to `npm start` working.

## 0. A fresh project with one file

```sh
npx @alpha-star/sdk init hop
cd hop
npm install
```

The `blank` template splits a game into several files. Hop keeps everything in `src/main.ts`, so
delete the rest of `src/` and the starter art:

```sh
rm -r src/alpha.ts src/types.ts src/tuning.ts src/prefabs src/scenes assets tools
```

(On Windows PowerShell: `Remove-Item -Recurse src/alpha.ts, src/types.ts, src/tuning.ts, src/prefabs, src/scenes, assets, tools`.)

Every step below adds to `src/main.ts`. Run `npm start` after each one; the window hot-reloads when
you save. The complete file is at the end of the page.

## 1. Names first

The engine type-checks every name: actions, sounds, tags, scenes. Declare them once in an
interface and bind `scene` / `game` to it with `define`. A typo such as `ctx.find("hoper")` is
then a compile error instead of a bug.

```ts
import { define, type Context, type EntitySpec } from "alpha";

interface Hop {
  actions: "hop";                          // input actions (bound to keys in config)
  sounds: "hop" | "point" | "crash";       // assets.sounds names
  images: never;                           // no images: shapes only
  tags: { hopper: {}; column: {}; gap: {}; floor: {}; label: {} };  // tag -> its data ({} = none)
  ui: {};                                  // no RmlUi documents
  scenes: { title: undefined; play: undefined; over: { score: number } };  // scene -> params
}

const { scene, game } = define<Hop>();
```

Put the numbers that shape the feel together, so tuning never means hunting through scenes:

```ts
const W = 320;
const H = 180;
const FLOOR_Y = 168;
const HOPPER = { x: 80, size: 10, hop: -230 };
const COLUMNS = { width: 22, gap: 56, speed: 90, every: 1.6, margin: 24 };
```

## 2. A floor and a falling square

Entities are plain objects (**spawn specs**): `tags`, `pos` (the center), a `shape` to draw and an
optional `body` for physics. Functions that return specs are called **prefabs**; `EntitySpec<Hop, "floor">`
is a spec for an entity tagged `floor` (its `data`, shapes and layers are checked against `Hop`).

```ts
function floor(): EntitySpec<Hop, "floor"> {
  return {
    tags: ["floor"],
    pos: [W / 2, (FLOOR_Y + H) / 2],
    shape: { rect: [W, H - FLOOR_Y], color: "#2a9d8f" },
    body: { type: "static", collider: { rect: [W, H - FLOOR_Y] } },   // never moves
  };
}

function hopper(): EntitySpec<Hop, "hopper"> {
  return {
    tags: ["hopper"],
    pos: [HOPPER.x, H / 2],
    z: 2,                                                             // drawn over columns
    shape: { rect: [HOPPER.size, HOPPER.size], color: "#ffd166" },
    body: { type: "dynamic", velocity: [0, HOPPER.hop], collider: { rect: [HOPPER.size, HOPPER.size] }, fixedRotation: true },
  };
}

const play = scene("play", {
  enter(ctx) {                 // runs once when the scene starts
    ctx.spawn(floor());
    ctx.spawn(hopper());
  },
});

export default game({
  config: { title: "Hop", size: [W, H], background: "#1d3557", seed: 7, gravity: [0, 700], actions: { hop: ["Space"] } },
  start: "play",
  scenes: { play },
});
```

**Expected:** the square jumps up once (its starting `velocity`), falls under `gravity` and lands
on the floor. `dynamic` bodies are moved by physics; `static` ones never move.

## 3. Hop on Space, with sound and sparks

`update` runs every fixed step (60 per second). `ctx.input.pressed(action)` is true on the step
the key goes down. Setting `body.velocity` makes the hop independent of how fast it was falling.

Bind more keys to the action, and declare synthesized sounds (a waveform swept between two
frequencies, so no files are needed):

```ts
  config: {
    // ...
    actions: { hop: ["Space", "Up", "W", "MouseLeft", "GamepadA"] },
  },
  assets: {
    sounds: {
      hop: { wave: "square", freq: [520, 780], duration: 0.07, volume: 0.25 },
      point: { wave: "sine", freq: [880, 1320], duration: 0.1, volume: 0.3 },
      crash: { wave: "noise", freq: [400, 80], duration: 0.3, volume: 0.35 },
    },
  },
```

Give the hopper a particle **emitter** (it emits nothing until asked) and burst it on each hop:

```ts
    // in hopper(), next to body:
    emitter: { lifetime: [0.2, 0.4], speed: [40, 90], angle: [100, 260], size: [3, 1], color: ["#ffd166", "#ef476f00"], max: 32 },
```

```ts
  update(ctx) {
    const h = ctx.find("hopper");
    if (ctx.input.pressed("hop")) {
      h.body.velocity = [0, HOPPER.hop];
      h.emit(12);                       // 12 sparks from the emitter
      ctx.audio.play("hop");
    }
    h.rotation = Math.max(-25, Math.min(60, h.body.velocity.y * 0.15));  // tilt with speed (degrees)
  },
```

Ranges like `[0.2, 0.4]` pick a random value per particle; pairs like `size: [3, 1]` and the two
colors go from start to end of each particle's life (`#ef476f00` fades to transparent).

## 4. Columns that scroll in

A **kinematic** body moves at its `velocity` and ignores gravity: ideal for obstacles. Each pair
is two columns plus an invisible **sensor** in the gap (a sensor reports overlaps but nothing
bounces off it). That sensor is how we score.

```ts
function column(x: number, top: number, bottom: number): EntitySpec<Hop, "column"> {
  const size: [number, number] = [COLUMNS.width, bottom - top];
  return {
    tags: ["column"],
    pos: [x, (top + bottom) / 2],
    shape: { rect: size, color: "#264653", outline: "#e9c46a" },
    body: { type: "kinematic", velocity: [-COLUMNS.speed, 0], collider: { rect: size } },
  };
}

function columnPair(gapY: number): EntitySpec<Hop, "column" | "gap">[] {
  const x = W + COLUMNS.width;            // just off the right edge
  const half = COLUMNS.gap / 2;
  return [
    column(x, 0, gapY - half),
    column(x, gapY + half, FLOOR_Y),
    { tags: ["gap"], pos: [x, gapY], body: { type: "kinematic", velocity: [-COLUMNS.speed, 0], collider: { rect: [4, COLUMNS.gap], sensor: true } } },
  ];
}
```

Spawn a pair now and then on a timer. `ctx.random` is seeded by `config.seed`, so every run (and
every test) sees the same gaps:

```ts
  enter(ctx) {
    // ...
    const spawnPair = () => {
      const half = COLUMNS.gap / 2;
      const gapY = ctx.random.range(COLUMNS.margin + half, FLOOR_Y - COLUMNS.margin - half);
      for (const spec of columnPair(gapY)) ctx.spawn(spec);
    };
    spawnPair();
    ctx.every(COLUMNS.every, spawnPair);   // scene timers stop when the scene ends
  },
```

Destroy columns once they leave the screen, in `update`:

```ts
    for (const c of ctx.findAll("column")) if (c.pos.x < -COLUMNS.width) ctx.destroy(c);
```

## 5. Crashing and scoring

`ctx.onHit(tagA, tagB, fn)` runs when two solid bodies touch; `ctx.onEnter` when a body enters a
sensor. The score is drawn as **world text**: a `text` shape, changed later through `entity.text`.

```ts
function label(text: string, y: number, size = 16): EntitySpec<Hop, "label"> {
  return { tags: ["label"], pos: [W / 2, y], z: 10, shape: { text, size, color: "#ffffff" } };
}

type Play = Context<Hop, { score: number }>;

function crash(ctx: Play) {
  ctx.audio.play("crash");
  ctx.camera.shake(4, 0.3);                      // 4 px for 0.3 s
  ctx.go("over", { score: ctx.state.score });   // next scene, with its params
}

const play = scene("play", {
  state: () => ({ score: 0 }),                   // fresh every time the scene starts

  enter(ctx) {
    ctx.spawn(floor());
    ctx.spawn(hopper());
    const score = ctx.spawn(label("0", 20));

    ctx.onHit("hopper", "column", () => crash(ctx));
    ctx.onHit("hopper", "floor", () => crash(ctx));
    ctx.onEnter("hopper", "gap", (_h, gap) => {
      ctx.state.score += 1;
      score.text = String(ctx.state.score);
      ctx.audio.play("point");
      ctx.destroy(gap);                          // score each gap once
    });
    // ... the column timer from step 4
  },
```

And in `update`, flying off the top also ends the run: `if (h.pos.y < 0) crash(ctx);`

## 6. Title and game over, with a saved best score

Scenes are the game's screens. `ctx.go(name, params)` switches; the typed params arrive in the next
scene's `enter`. `ctx.save` is a small key/value store written to disk at once.

```ts
const title = scene("title", {
  enter(ctx) {
    ctx.spawn(floor());
    ctx.spawn(label("HOP", 60, 32));
    ctx.spawn(label("press Space to start", 110, 8));
    ctx.spawn(label(`best ${ctx.save.get("best", 0)}`, 130, 8));
  },
  update(ctx) {
    if (ctx.input.pressed("hop")) ctx.go("play");
  },
});

const over = scene("over", {
  enter(ctx, { score }) {
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    ctx.spawn(floor());
    ctx.spawn(label("GAME OVER", 60, 24));
    ctx.spawn(label(`score ${score}   best ${best}`, 100, 8));
    ctx.spawn(label("press Space", 125, 8));
  },
  update(ctx) {
    if (ctx.time > 0.4 && ctx.input.pressed("hop")) ctx.go("play");   // a short pause first
  },
});
```

Register them and start on the title: `start: "title", scenes: { title, play, over }`.

## 7. Tests that play the game for you

`simulate` runs the game headless, frame by frame, with scripted input; the same seed and inputs
give the same result every time. Replace `test/main.test.ts`:

```ts
// Headless tests (`npm test`): same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import game from "../src/main";

// Frames at 60 per second: a hop every 39 frames (rise and fall take ~0.66 s) holds the height.
const STEADY = Array.from({ length: 12 }, (_, i) => i * 39);

export function titleWaitsForSpace() {
  simulate(game, { seconds: 1 }).expect.scene("title");
}

export function withoutHoppingTheHopperFallsAndTheRunEnds() {
  simulate(game, { scene: "play", seconds: 2 }).expect.scene("over");
}

export function columnsArriveInPairsWithAGap() {
  const run = simulate(game, { scene: "play", seconds: 1, inputs: { hop: STEADY } });
  run.expect.scene("play");
  run.expect.count("column").equals(2);
  run.expect.count("gap").equals(1);
}

export function noErrorsWhileHopping() {
  const run = simulate(game, { scene: "play", seconds: 3, inputs: { hop: STEADY } });
  run.expect.noErrors();
}
```

```sh
npm test
```

**Expected:** `4 passed, 0 failed`. Change `HOPPER.hop` to `-150` and run again: the steady-hop
test now fails, which is how tests catch a tuning change that breaks the game.

## 8. Ship it

```sh
npx alpha export --standalone      # a folder with the engine and your game, ready to zip
```

See [Exporting](exporting.md) and [Android](android.md).

## The complete file

`src/main.ts`, exactly as tested:

```ts
// Hop: tap to hop a little square through gaps between columns.
// The whole game in one file, drawn with shapes only (no images).
import { define, type Context, type EntitySpec } from "alpha";

// 1. Names: every action, sound, tag and scene the game uses.
interface Hop {
  actions: "hop";
  sounds: "hop" | "point" | "crash";
  images: never;
  tags: { hopper: {}; column: {}; gap: {}; floor: {}; label: {} };  // tag -> its data ({} = none)
  ui: {};
  scenes: { title: undefined; play: undefined; over: { score: number } };
}

const { scene, game } = define<Hop>();

// 2. Tuning: every number that changes the feel, in one place.
const W = 320;
const H = 180;
const FLOOR_Y = 168;
const HOPPER = { x: 80, size: 10, hop: -230 };
const COLUMNS = { width: 22, gap: 56, speed: 90, every: 1.6, margin: 24 };

// 3. Prefabs: functions that return spawn specs.
function hopper(): EntitySpec<Hop, "hopper"> {
  return {
    tags: ["hopper"],
    pos: [HOPPER.x, H / 2],
    z: 2,
    shape: { rect: [HOPPER.size, HOPPER.size], color: "#ffd166" },
    // Starts with one hop, so the player has a moment before the first fall.
    body: { type: "dynamic", velocity: [0, HOPPER.hop], collider: { rect: [HOPPER.size, HOPPER.size] }, fixedRotation: true },
    // A puff of sparks; play() bursts it on every hop.
    emitter: { lifetime: [0.2, 0.4], speed: [40, 90], angle: [100, 260], size: [3, 1], color: ["#ffd166", "#ef476f00"], max: 32 },
  };
}

function floor(): EntitySpec<Hop, "floor"> {
  return {
    tags: ["floor"],
    pos: [W / 2, (FLOOR_Y + H) / 2],
    shape: { rect: [W, H - FLOOR_Y], color: "#2a9d8f" },
    body: { type: "static", collider: { rect: [W, H - FLOOR_Y] } },
  };
}

// A column moving left: kinematic bodies move at their velocity and ignore gravity.
function column(x: number, top: number, bottom: number): EntitySpec<Hop, "column"> {
  const size: [number, number] = [COLUMNS.width, bottom - top];
  return {
    tags: ["column"],
    pos: [x, (top + bottom) / 2],
    shape: { rect: size, color: "#264653", outline: "#e9c46a" },
    body: { type: "kinematic", velocity: [-COLUMNS.speed, 0], collider: { rect: size } },
  };
}

// Two columns and an invisible sensor in the gap that scores once.
function columnPair(gapY: number): EntitySpec<Hop, "column" | "gap">[] {
  const x = W + COLUMNS.width;
  const half = COLUMNS.gap / 2;
  return [
    column(x, 0, gapY - half),
    column(x, gapY + half, FLOOR_Y),
    { tags: ["gap"], pos: [x, gapY], body: { type: "kinematic", velocity: [-COLUMNS.speed, 0], collider: { rect: [4, COLUMNS.gap], sensor: true } } },
  ];
}

function label(text: string, y: number, size = 16): EntitySpec<Hop, "label"> {
  return { tags: ["label"], pos: [W / 2, y], z: 10, shape: { text, size, color: "#ffffff" } };
}

// 4. Scenes.
const title = scene("title", {
  enter(ctx) {
    ctx.spawn(floor());
    ctx.spawn(label("HOP", 60, 32));
    ctx.spawn(label("press Space to start", 110, 8));
    ctx.spawn(label(`best ${ctx.save.get("best", 0)}`, 130, 8));
  },
  update(ctx) {
    if (ctx.input.pressed("hop")) ctx.go("play");
  },
});

type Play = Context<Hop, { score: number }>;

function crash(ctx: Play) {
  ctx.audio.play("crash");
  ctx.camera.shake(4, 0.3);
  ctx.go("over", { score: ctx.state.score });
}

const play = scene("play", {
  state: () => ({ score: 0 }),

  enter(ctx) {
    ctx.spawn(floor());
    ctx.spawn(hopper());
    const score = ctx.spawn(label("0", 20));

    // Collisions: solid hits end the run, entering a gap scores.
    ctx.onHit("hopper", "column", () => crash(ctx));
    ctx.onHit("hopper", "floor", () => crash(ctx));
    ctx.onEnter("hopper", "gap", (_h, gap) => {
      ctx.state.score += 1;
      score.text = String(ctx.state.score);
      ctx.audio.play("point");
      ctx.destroy(gap);
    });

    // A new pair on a timer; ctx.random is seeded, so tests see the same gaps every run.
    const spawnPair = () => {
      const half = COLUMNS.gap / 2;
      const gapY = ctx.random.range(COLUMNS.margin + half, FLOOR_Y - COLUMNS.margin - half);
      for (const spec of columnPair(gapY)) ctx.spawn(spec);
    };
    spawnPair();
    ctx.every(COLUMNS.every, spawnPair);
  },

  update(ctx) {
    const h = ctx.find("hopper");
    if (ctx.input.pressed("hop")) {
      h.body.velocity = [0, HOPPER.hop];
      h.emit(12);
      ctx.audio.play("hop");
    }
    h.rotation = Math.max(-25, Math.min(60, h.body.velocity.y * 0.15));  // tilt with speed
    if (h.pos.y < 0) crash(ctx);  // flew off the top

    // Columns that left the screen are gone for good.
    for (const c of ctx.findAll("column")) if (c.pos.x < -COLUMNS.width) ctx.destroy(c);
  },
});

const over = scene("over", {
  enter(ctx, { score }) {
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);  // written to disk at once
    ctx.spawn(floor());
    ctx.spawn(label("GAME OVER", 60, 24));
    ctx.spawn(label(`score ${score}   best ${best}`, 100, 8));
    ctx.spawn(label("press Space", 125, 8));
  },
  update(ctx) {
    // A short pause so a hop held from the crash does not restart at once.
    if (ctx.time > 0.4 && ctx.input.pressed("hop")) ctx.go("play");
  },
});

// 5. The game: config, scenes and synthesized sounds (no audio files needed).
export default game({
  config: {
    title: "Hop",
    size: [W, H],
    background: "#1d3557",
    seed: 7,
    gravity: [0, 700],
    actions: { hop: ["Space", "Up", "W", "MouseLeft", "GamepadA"] },
  },
  start: "title",
  scenes: { title, play, over },
  assets: {
    sounds: {
      hop: { wave: "square", freq: [520, 780], duration: 0.07, volume: 0.25 },
      point: { wave: "sine", freq: [880, 1320], duration: 0.1, volume: 0.3 },
      crash: { wave: "noise", freq: [400, 80], duration: 0.3, volume: 0.35 },
    },
  },
});
```

## Where to go next

- Swap shapes for images and animated sprite sheets: [Sprites, camera, LDtk levels](tilemaps.md).
- A HUD and menus with buttons: [Game UI](ui.md).
- Music, volume settings and languages: [Audio](audio.md), [Saves, settings, i18n](saves.md).
- The bigger version of this game, split into files: the `flappy` example in [Examples](examples.md).
