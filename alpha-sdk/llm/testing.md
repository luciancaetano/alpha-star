# Testing

> summary: Headless deterministic tests with alpha/test simulate(), its options (inputs, clicks, saves, settings, appPauses) and checks (expect, entities, state); read before writing or debugging test/main.test.ts.

`alpha test` (npm test) type-checks and bundles `test/main.test.ts`, then runs every exported function headlessly. A test fails by throwing.

## API

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

simulate(game, {
  seconds: 1.5,                                   // round(seconds * 60) fixed steps of 1/60 s
  scene?: "play",                                 // default: game's start
  inputs?: { jump: [30, 60], axes: { moveX: [[0, 1], [90, 0]] } },  // or an array, one per player
  clicks?: { "solved#next": [120] },              // "doc#id" -> frames
  saves?: { main: { level: 1 } },                 // boot save slots (main = ctx.save)
  settings?: { locale: "pt", reducedMotion: true, volumes: { music: 0.5 } },
  appPauses?: [[60, 120]],                        // frames paused: onPause at from, onResume at to
}): Run

run.expect.scene(name); run.expect.noErrors();
run.expect.count(tag).equals(n) / .atLeast(n);
run.expect.text(doc, id).equals(s); run.expect.shown(doc);
run.expect.animation(tag).equals(name | "");     // oldest living entity with the tag
run.expect.particles().equals(n) / .atLeast(n);
run.expect.music(name | null);
run.frames; run.state;                           // scene state as JSON copy
run.entities(tag)                                // [{ id, pos, rotation, scale, alpha, data, velocity | null }]
run.music; run.locale; run.settings; run.saves;
```

## Example (examples/platformer/test/main.test.ts)

```ts
const RIGHT = { axes: { moveX: [[0, 1]] as Array<[number, number]> } };

export function playerLandsAndIdles() {
  const run = simulate(game, { seconds: 1.5 });
  run.expect.noErrors();
  run.expect.animation("player").equals("idle");
  run.expect.count("coin").equals(8);
}

export function runningRightCollectsTheFirstCoins() {
  const run = simulate(game, { seconds: 1.5, inputs: RIGHT });
  run.expect.count("coin").equals(6);
  run.expect.text("hud", "coins").equals("coins 2/8");
}
```

## Rules and gotchas

- Deterministic: same game + options => same run on every machine. Each call is a fresh game (new world and timers, random from `config.seed`, in-memory saves/settings, no window, no audio output).
- Frames are 0-based; action frames press for one step; axis entries hold until the next entry.
- Axis tuple arrays need `as Array<[number, number]>` when declared outside the call (TS infers `number[][]`).
- A script error stops the run; call `expect.noErrors()` to surface it.
- Clicking a hidden document or unknown id fails the test.
- Postfx/lighting/audio output are invisible to tests (render/output only).
- Pure logic (e.g. examples/puzzle `rules.ts`) can be tested as plain function calls in the same file.
- Workflow: write the failing test first, see it fail for the expected reason, then make it pass. Test what the player sees (scenes, counts, texts, positions), not code structure.
- `alpha/test` exists only in release/dev engines; never import it from `src/`.
