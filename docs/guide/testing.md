# Testing

`alpha test` type-checks and bundles `test/main.test.ts`, then runs every exported function
headlessly in the engine. Each `simulate` call is a fresh game: new world, random seeded from
`config.seed`, in-memory saves and settings, no window, no audio output.

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

export function holdingRightRuns() {
  const run = simulate(game, { seconds: 1.5, inputs: { axes: { moveX: [[0, 1]] } } });
  run.expect.noErrors();
  run.expect.animation("player").equals("run");
}
```

## Options

| Option | Meaning |
|---|---|
| `seconds` | Simulated time: `round(seconds * 60)` fixed steps |
| `scene` | Start scene (default: the game's `start`) |
| `inputs` | `{ action: [frames], axes: { axis: [[frame, value]] } }`, or an array per player |
| `clicks` | `{ "doc#id": [frames] }` |
| `saves` / `settings` | Boot state, e.g. `{ main: { best: 5 } }`, `{ locale: "pt" }` |
| `appPauses` | `[[from, to]]` frames the app is paused (focus lost) |
| `platform` | the os `ctx.platform` reports (`"android"`, `"windows"`, ...; default: the machine running the test) |
| `size` | screen pixels to run on, e.g. a tall phone `[1080, 2340]`: `ctx.camera.view`, `ctx.safeArea` and the UI grow as `config.window.aspect` says (default: `config.size`) |

## Checks

`run.expect.scene(name)`, `noErrors()`, `count(tag).equals(n)` / `atLeast(n)`,
`text(doc, id).equals(s)`, `shown(doc)`, `animation(tag).equals(name)`, `particles()`,
`music(name | null)`. Plain data: `run.entities(tag)` (pos, rotation, scale, alpha, data,
velocity), `run.state`, `run.saves`, `run.settings`, `run.locale`, `run.music`. A test fails by
throwing.

Pure game logic can be tested directly as plain function calls in the same file
(`examples/puzzle` tests its rules that way).

Write the failing test first, see it fail for the expected reason, then make it pass. Test what
the player sees (scenes, counts, texts, positions), not how the code is arranged.
