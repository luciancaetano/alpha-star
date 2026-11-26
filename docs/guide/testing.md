# Testing

`npm test` (which runs `alpha test`) plays your game **headlessly**: no window, no sound, as fast
as the CPU allows. Every exported function in `test/main.test.ts` is one test. Each test calls
`simulate(game, options)`, which boots a fresh copy of the game, feeds it scripted input for a
number of seconds, and hands back what happened so you can check it.

Runs are deterministic: the same game and the same options give the same result, every time, on
every machine. That makes tests exact instead of flaky.

## A complete test file

This file runs against the `blank` template (`alpha init my-game --template blank`) as is. Copy it
over `test/main.test.ts` and run `npm test`.

```ts
// test/main.test.ts: every exported function is one test. `npm test` runs them all headlessly.
import { simulate, type SimulateOptions } from "alpha/test";
import type { Game } from "../src/types";
import game from "../src/main";

// A tiny helper: a test fails by throwing.
function check(ok: boolean, what: string) {
  if (!ok) throw new Error(what);
}

// Frames from..to-1: "hold this action" for a stretch of steps.
const frames = (from: number, to: number) => Array.from({ length: to - from }, (_, i) => from + i);

export function startsCleanWithOnePlayer() {
  const run = simulate(game, { seconds: 0.5 });
  run.expect.noErrors();
  run.expect.scene("play");
  run.expect.count("player").equals(1);
}

export function holdingRightMovesThePlayer() {
  const run = simulate(game, { seconds: 0.5, inputs: { axes: { moveX: [[0, 1]] } } });
  const [p] = run.entities("player");
  check(p.pos[0] > 210, `expected x > 210, got ${p.pos[0]}`);
}

export function releasingTheAxisStopsThePlayer() {
  const a = simulate(game, { seconds: 1, inputs: { axes: { moveX: [[0, 1], [30, 0]] } } }).entities("player")[0];
  const b = simulate(game, { seconds: 2, inputs: { axes: { moveX: [[0, 1], [30, 0]] } } }).entities("player")[0];
  check(a.pos[0] === b.pos[0], `player kept moving: ${a.pos[0]} -> ${b.pos[0]}`);
}

export function pressingJumpSquashesThePlayer() {
  const mid = simulate(game, { seconds: 0.1, inputs: { jump: [0] } }).entities("player")[0];
  check(mid.scale > 1, `scale should grow, got ${mid.scale}`);
}

export function mashingJumpNeverBreaksTheGame() {
  simulate(game, { seconds: 3, inputs: { jump: frames(0, 180) } }).expect.noErrors();
}

export function sameInputsSameResult() {
  const opts: SimulateOptions<Game> = { seconds: 2, inputs: { jump: [10, 40], axes: { moveX: [[0, 1], [50, -1]] } } };
  const a = JSON.stringify(simulate(game, opts).entities("player"));
  const b = JSON.stringify(simulate(game, opts).entities("player"));
  check(a === b, "two runs with the same inputs differ");
}

export function runsOnATallPhone() {
  const run = simulate(game, { seconds: 0.5, size: [1080, 2340], platform: "android" });
  run.expect.noErrors();
  run.expect.count("player").equals(1);
}

export function pausingTheAppStopsTime() {
  // Frames 10..70 the app is in the background: no steps run.
  const run = simulate(game, { seconds: 1, inputs: { axes: { moveX: [[0, 1]] } }, appPauses: [[10, 70]] });
  run.expect.noErrors();
}

export function settingsAndSavesBoot() {
  const run = simulate(game, { seconds: 0.1, saves: { main: { best: 7 } }, settings: { reducedMotion: true } });
  check(run.saves.main.best === 7, JSON.stringify(run.saves));
  check(run.settings.reducedMotion, JSON.stringify(run.settings));
}
```

**Expected output:**

```
PASS holdingRightMovesThePlayer
PASS mashingJumpNeverBreaksTheGame
...
PASS startsCleanWithOnePlayer
9 passed, 0 failed
```

A failing check names what it expected and where:

```
FAIL deliberatelyFails
    Error: expected 2 "player" after 6 frames, got 1
        at deliberatelyFails (test/main.test.ts:68:95)
9 passed, 1 failed
```

`npm test` exits non-zero when any test fails, so it works as a CI step as is.

## Step by step

### 1. Time: `seconds` and frames

The engine runs a fixed step of 1/60 s. `seconds: 1.5` runs `round(1.5 * 60)` = 90 steps. Every
frame number in the other options counts those steps from **0**: frame 60 is one second in.

```ts
simulate(game, { seconds: 2 });                 // 120 steps from the game's `start` scene
simulate(game, { scene: "play", seconds: 2 });  // skip the title: start in "play"
```

`scene` starts in any scene; its `enter` gets no params (`undefined`).

### 2. Pressing and holding actions

`inputs` maps each **action** (from `config.actions`) to the frames it is pressed on. Each listed
frame is one step with the action down, so `ctx.input.pressed()` fires on it.

```ts
simulate(game, { seconds: 1, inputs: { jump: [0] } });            // tap jump on the first step
simulate(game, { seconds: 2, inputs: { jump: [0, 60] } });        // tap at 0 s and at 1 s
simulate(game, { seconds: 1, inputs: { fire: frames(0, 30) } });  // hold fire for half a second
```

To **hold** an action, list consecutive frames (the `frames` helper above). Between two listed
frames that are not consecutive, the action is released, so each one is a new press.

### 3. Moving with axes

`inputs.axes` maps each **axis** (from `config.axes`) to `[frame, value]` pairs. The value
(-1..1) stays until the next pair.

```ts
// Right from the start, stop at 0.5 s, left from 1 s.
simulate(game, { seconds: 2, inputs: { axes: { moveX: [[0, 1], [30, 0], [60, -1]] } } });
```

Both go together in one object: `{ jump: [50], axes: { moveX: [[0, 1], [52, 0]] } }`
(from `examples/platformer`: run right, jump at frame 50, stop at 52).

### 4. Several players

Pass an array, one entry per player (1..4). The game reads them through `ctx.input.player(n)`:

```ts
simulate(game, { seconds: 1, inputs: [{ axes: { moveX: [[0, 1]] } }, { axes: { moveX: [[0, -1]] } }] });
```

### 5. Clicking UI buttons

`clicks` maps `"document#elementId"` to the frames it is clicked on, before that step runs. A
hidden document or an unknown id fails the test, so a click that cannot happen never passes
silently. From `examples/flappy`:

```ts
export function settingsMenuSetsVolumeAndLanguage() {
  const run = simulate(game, {
    seconds: 0.3,
    inputs: { settings: [0] },                    // S / Start opens the menu
    clicks: { "settings#music-down": [5, 6], "settings#sfx-up": [7], "settings#language": [8] },
  });
  run.expect.scene("settings");
  run.expect.text("settings", "music").equals("80%");
  if (run.settings.volumes.music !== 0.8 || run.locale !== "pt") throw new Error(JSON.stringify(run.settings));
}
```

### 6. Checking results: `run.expect`

| Check | Passes when, at the end of the run |
|---|---|
| `noErrors()` | no script error happened (a script error also stops the run) |
| `scene("over")` | that scene is current |
| `count("enemy").equals(3)` / `.atLeast(3)` | that many living entities carry the tag |
| `text("hud", "score").equals("12")` | the UI element's plain text matches |
| `shown("hud")` | the UI document is shown |
| `animation("player").equals("run")` | the oldest entity with the tag plays that animation (`""` = none) |
| `particles().equals(0)` / `.atLeast(5)` | live particles across every emitter |
| `music("theme")` / `music(null)` | that music is playing / none is |

Names are type-checked against `src/types.ts`: a typo in a tag, scene, UI id or animation is a
compile error, not a test that passes by accident.

### 7. Reading plain data

When a ready-made check does not fit, read the data and throw:

| Field | Holds |
|---|---|
| `run.entities(tag)` | living entities, oldest first: `{ id, pos, rotation, scale, alpha, data, velocity }` (`velocity` is `null` without a body) |
| `run.state` | the scene's `ctx.state` (a JSON copy) |
| `run.saves` | every save slot: `run.saves.main` is `ctx.save` |
| `run.settings` | `{ volumes, locale, uiScale, highContrast, reducedMotion, bindings }` |
| `run.locale`, `run.music` | the current locale and music (or `null`) |
| `run.frames` | steps run |

`run.state` is typed `unknown`; cast it to the shape your scene declares:

```ts
const score = (state: unknown) => (state as { score: number }).score;   // examples/topdown
if (score(run.state) < 2) throw new Error(`expected some kills, score ${score(run.state)}`);
```

### 8. Boot state: saves and settings

Each run starts with empty in-memory saves and default settings. Seed them to test a returning
player. From `examples/puzzle` and `examples/flappy`:

```ts
simulate(game, { seconds: 0.2, saves: { main: { level: 1 } } }).expect.text("hud", "level").equals("LEVEL 2/3");
simulate(game, { seconds: 0.1, settings: { locale: "pt" } }).expect.text("title", "hint").equals("aperte para voar");
```

`settings` takes `volumes`, `locale`, `fullscreen`, `bindings`, `uiScale`, `highContrast` and
`reducedMotion`, as `settings.json` holds them. Nothing is read from or written to disk.

### 9. Platform and screen size

`platform` sets what `ctx.platform.os` reports (`ctx.platform.mobile` follows it); `size` runs on
a screen of that many pixels, so `ctx.camera.view`, `ctx.safeArea` and the UI grow the way
`config.window.aspect` says. Defaults: the machine running the test, and `config.size`.

```ts
// examples/flappy: the hint follows the platform.
simulate(game, { seconds: 0.1, platform: "windows" }).expect.text("title", "hint").equals("press flap");
simulate(game, { seconds: 0.1, platform: "android" }).expect.text("title", "hint").equals("tap to flap");

// A 1080x2340 phone shows more world; the pipes (the difficulty) must not change.
const phone = simulate(game, { scene: "play", seconds: 3.1, size: [1080, 2340], platform: "android" });
```

### 10. App pauses

`appPauses: [[from, to]]` puts the app in the background between those frames (focus lost,
phone switched app): `ctx.onPause` runs at `from`, no steps run, `ctx.onResume` runs at `to`.

## Recipes

**Test pure rules without the engine.** Logic that is a plain function needs no `simulate`
(`examples/puzzle` tests its Sokoban rules this way):

```ts
import { move, parse } from "../src/rules";

export function twoBoxesInARowCannotBePushed() {
  const { level, start } = parse(["#####", "#@$$#", "#####"]);
  if (move(level, start, "right") !== null) throw new Error("pushed two boxes");
}
```

**Check something at two moments.** Each run is fresh, so run twice with different `seconds`
(from `examples/platformer`):

```ts
const at = (seconds: number) => simulate(game, { seconds }).entities("lift")[0].pos[0];
if (Math.abs(at(2.5) - 470) > 1) throw new Error(`lift should reach x 470 at 2.5 s, got ${at(2.5)}`);
if (Math.abs(at(5) - 340) > 1) throw new Error(`lift should be back at x 340 at 5 s, got ${at(5)}`);
```

**Assert an invariant over a long run.** Mash inputs for seconds and check every entity:

```ts
const run = simulate(game, { seconds: 3, inputs: { jump: [60, 120], axes: { moveX: [[0, 1]] } } });
for (const p of run.entities("player")) if (p.rotation !== 0) throw new Error(`player rotated to ${p.rotation}`);
```

**Full play-through.** Script a whole level and check the scene and the save:

```ts
const run = simulate(game, { seconds: 1, inputs: { right: [5] } });   // examples/puzzle, level 1
run.expect.scene("solved");
if (run.saves.main.best0 !== 1) throw new Error(JSON.stringify(run.saves));
```

**Effects clear themselves.** Particles and popups should be gone after a while:

```ts
simulate(game, { seconds: 0.3 }).expect.particles().atLeast(5);
simulate(game, { seconds: 1 }).expect.particles().equals(0);
```

## Gotchas

- **Shared option objects need a type.** `const opts = { ... } as const` makes the arrays
  readonly and fails to compile. Annotate instead: `const opts: SimulateOptions<Game> = { ... }`
  (import `SimulateOptions` from `"alpha/test"` and `Game` from `../src/types`).
- **Frames start at 0** and the last frame of `seconds: 1` is 59. An input listed past the end
  never happens.
- **Randomness comes from `config.seed`.** Use `ctx.random` in the game, never `Math.random`, or
  runs stop being reproducible.
- **Positions are exact numbers.** Compare with a tolerance when physics is involved
  (`Math.abs(y - 113) > 1`).
- **Test what the player sees** (scenes, counts, texts, positions), not how the code is arranged:
  a test that would still pass if the game did nothing is not a test.
- **Write the failing test first**, see it fail for the expected reason, then make it pass.

## Commands

```
npm test                                   # = alpha test: test/main.test.ts
npx alpha test --entry test/levels.test.ts # another test file
npx alpha test --engine path/to/alpha_star # a specific engine build
```

Agents run the same tests through the MCP tool `alpha_test` ([MCP for agents](mcp.html)).

## See also

- [Input](input.html): actions, axes and players the tests drive.
- [Game UI](ui.html): the documents and ids `clicks` and `text` use.
- [MCP for agents](mcp.html): play the real game step by step instead of headlessly.
- [Examples](examples.html): every example has a `test/main.test.ts` worth reading.
- [API reference](reference.html): `simulate`, `SimulateOptions`, `Run`, `Expect`.
