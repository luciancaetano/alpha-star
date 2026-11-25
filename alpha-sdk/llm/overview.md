# Overview

> summary: Project layout, the types.ts/alpha.ts/scenes/main.ts pattern, GameConfig, the scene lifecycle and ctx, and the alpha CLI; read this first before writing any alpha-star game code.

The API is `node_modules/@alpha-star/sdk/types/alpha.d.ts` (modules `"alpha"` and `"alpha/test"`). Read it when unsure. Units: pixels, seconds, degrees. Origin top-left, +y down.

## Layout (every template/example)

| File | Holds |
|---|---|
| `src/types.ts` | The game's `GameTypes` interface: every name (actions, tags, sounds, scenes...). Edit first. |
| `src/alpha.ts` | `export const { scene, game } = define<MyGame>();` |
| `src/tuning.ts` | Numbers that set the feel. |
| `src/prefabs/*.ts` | Functions returning `EntitySpec` (plain data). |
| `src/scenes/*.ts` | `export default scene("play", { state, enter, update, exit })` |
| `src/main.ts` | `export default game({ config, start, scenes, assets })` (entry point) |
| `test/main.test.ts` | Exported functions using `simulate` (see testing.md). |
| `AGENTS.md` | What an agent reads first: commands, layout, game-specific rules. |

## GameTypes (types.ts)

```ts
export interface Platformer {
  actions: "jump" | "restart";
  axes: "moveX";                       // optional
  sounds: "jump" | "coin";
  images: never;                       // `never` if none
  tags: "player" | "coin" | "walls";
  sheets: { hero: "idle" | "run" };    // optional
  maps: "level1"; layers: "far" | "fx"; fonts: "pixel";   // optional
  ui: { hud: "coins"; won: never };    // `{}` if no UI
  scenes: { play: undefined; over: { score: number } };    // enter() params per scene
}
```
Other optional keys: `categories`, `music`, `locales`, `buses`. A misspelled name is a compile error.

## main.ts (from examples/platformer)

```ts
import { game } from "./alpha";
import play from "./scenes/play";
import won from "./scenes/won";
export default game({
  config: {
    title: "Platformer", size: [320, 180], background: "#87ceeb", seed: 1, gravity: [0, 900],
    actions: { jump: ["Space", "W", "Up", "GamepadA"], restart: ["R", "GamepadStart"] },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"] },
    window: { scaling: "integer" },
  },
  start: "play",                       // must be a scene with `undefined` params
  scenes: { play, won },               // every declared scene, exactly once
  assets: { sounds: { jump: { wave: "square", freq: [250, 500], duration: 0.1 } } },
});
```

`GameConfig`: `title, size, seed, actions` (required), `background?, gravity?, android?, axes?, deadzone?, collision?, pixelMode?, layers?, lighting?, window?, pauseOnFocusLoss?, audio?, save?, locale?`.
`assets`: `sounds` (required), `music?, locales?, images?, fonts?, ui?, sheets?, maps?`. Everything loads before the start scene; a missing file stops boot with its name.

## Scenes and ctx

```ts
import type { Context } from "alpha";
import { scene } from "../alpha";
type State = { coins: number };
type Play = Context<Platformer, State>;   // for helper functions

export default scene("play", {
  state: () => ({ coins: 0 }),         // (params) => S, fresh on every enter
  enter(ctx, params) { ctx.ui.show("hud"); },
  update(ctx) {                        // once per fixed step (1/60 s)
    if (ctx.input.pressed("restart")) ctx.go("play");
  },
  exit(ctx) {},
});
```

`ctx` (scene-scoped: entities, handlers, tweens, timers, camera, postfx cleared on scene change):
`config, dt, time, state, input, random, save, settings, locale, t, ui, audio, camera, postfx, window, app`,
`spawn, destroy, find, findAll, positions, spawnMap, map, onHit, onHitEnd, onEnter, onExit, onPause, onResume, onLowMemory, raycast, raycastAll, queryCircle, queryRect, joint, tween, every, after, go`.

- `ctx.go(scene, params?)` switches at the end of the step.
- `ctx.dt` is the constant fixed step; `ctx.time` = seconds since enter.
- `ctx.random.range(min, max)` float in [min, max), `.int(min, max)` inclusive, `.pick(items)`.
- `import { vec } from "alpha"`: `add sub scale len norm dot lerp angle fromAngle dist` on `[x, y]` (degrees, 0 = right, 90 = down).
- `console.log/warn/error`: stderr + F12 dev tools console.
- App lifecycle: `ctx.onPause/onResume/onLowMemory(fn)`, `ctx.app.paused`; `config.pauseOnFocusLoss` (default true). No catch-up after resume.

## CLI (run in the game folder)

```
alpha init <dir> [--template blank|platformer|topdown|puzzle|shmup]
alpha --version          SDK version (= the bundled engines' version)
alpha check              type-check only                 (npm run check)
alpha test               run test/main.test.ts headless  (npm test)
alpha export [--out dist] [--standalone] [--no-atlas]
alpha run [--watch] [-- engine args]   export then play; --watch hot-reloads
alpha mcp                MCP server for agents (mcp.md)
```
Done = `npm run check` clean and `npm test` passing.

## Rules

- Determinism: fixed step, `ctx.random` only (`Math.random` throws "use ctx.random"). Particles, tweens, camera, animations all advance on the fixed step. Audio never affects the simulation.
- A script error pauses the game with a source-mapped message; it never crashes the engine.
- Hot reload keeps the save, the scene and JSON params, and JSON `ctx.state` (if the scene still declares `state`); `enter` rebuilds the world. Keep long-lived values in `ctx.state`, not entity data.
- Dev tools (F12), MCP and `alpha/test` are absent from shipping engines: game code never depends on them.
