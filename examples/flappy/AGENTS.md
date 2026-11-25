# AGENTS.md — Flappy (alpha-star game)

You are editing a game for the alpha-star engine. Write TypeScript in `src/`;
`alpha-build` type-checks it and bundles it into one JS module (`dist/main.js`)
that the engine runs. The API types live in `../../alpha-sdk/types/alpha.d.ts`;
read that file when unsure, it is the source of truth.

## Commands

```
npm install                 # once
npm run check               # type-check only (fast)
npm run export              # type-check + bundle + copy assets -> dist/ (the pack)
npm start                   # export, then open the pack in the engine bundled in the SDK
npm test                    # type-check + run test/main.test.ts headlessly (alpha test)
npx alpha run -- --frames 120 --screenshot s.bmp   # headless check
npx alpha export --standalone --out build/game     # pack + engine binary: runs by itself
```

A change is done only when `npm run export` prints `exported dist (...)`.
Errors are `file:line:col: error: ...`; nothing is written while any exist.
Asset paths in `main.ts` are relative to this folder (e.g. `assets/sfx/hit.wav`);
the export copies each declared file into the pack and fails naming any missing one.

## Layout (one responsibility per file)

| File | Holds |
|---|---|
| `src/types.ts` | Every name: actions, sounds, tags, scenes + their params, UI documents + element ids. **Edit first.** |
| `src/alpha.ts` | `define<Flappy>()` → typed `scene` / `game`. Never edit. |
| `src/tuning.ts` | Gameplay numbers (sizes, speeds, timings). Change feel here, not in scenes. |
| `src/prefabs/*.ts` | Pure functions returning `EntitySpec` (plain data). |
| `src/scenes/*.ts` | One scene per file: `export default scene("name", {...})`. |
| `assets/ui/*.rml` | UI screens (RmlUi: HTML-like). `flappy.rcss` = shared CSS-like style. |
| `assets/fonts/` | Press Start 2P (OFL, license in `OFL.txt`), `font-family: pixel`. |
| `src/main.ts` | `game({ config, start, scenes, assets })`. Registers every scene. |
| `test/main.test.ts` | Headless `simulate()` tests. |

## How to add things

- **New input:** add to `actions` in `types.ts`, then map devices in `main.ts` `config.actions`.
- **New sound:** add to `sounds` in `types.ts`, then in `main.ts` `assets.sounds` either a WAV path
  (relative to this folder) or a synth spec `{ wave, freq, duration }` / list of notes.
  Play with `ctx.audio.play("name", { volume, pitch, loop })`; one-off procedural sounds with
  `ctx.audio.tone({ wave: "square", freq: [300, 600], duration: 0.1 })`.
- **New image:** add to `images` in `types.ts`, the file (PNG/JPG/BMP) in `main.ts` `assets.images`,
  then `shape: { image: "name", size: [w, h] }`. All assets preload before the first scene.
- **New entity kind:** add its tag to `tags`, write a prefab in `src/prefabs/`, `ctx.spawn(prefab())`.
- **New scene:** add `name: Params | undefined` to `scenes`, create `src/scenes/name.ts`,
  register it in `main.ts` `scenes`. The compiler lists what is missing.
- **UI (RmlUi):** screens are `.rml` documents styled with `.rcss` in `assets/ui/`. Declare each in
  `main.ts` `assets.ui` and its scripted element ids in `types.ts` `ui` (`never` = only shown/hidden).
  In a scene: `ctx.ui.show("hud")`, `ctx.ui.hide("hud")`, `ctx.ui.set("hud", "score", n)` (plain text),
  `ctx.ui.on("over", "again", "click", fn)` (mouse, touch, Enter/Space or gamepad A on focus) and
  `"back"` (Escape / gamepad B while shown). Scene change hides every document, reloads it and drops
  its handlers. RmlUi has no default stylesheet: give `div`/`p`/`h1` `display: block`; buttons need
  `tab-index: auto` (focusable by D-pad/stick/Tab) and a `:focus` style; `autofocus` on the button a
  controller user should land on. Fonts: `assets.fonts` name = `font-family`. Files a document links
  (`<link href>`, `src=`, `url()`) are packed automatically. A UI click/press the UI used never also
  triggers the game action (no double flap).
- **Collision:** in `enter`, `ctx.onHit("a", "b", fn)` (solid) or `ctx.onEnter("a", "b", fn)` (sensor).
- **Timed event:** `ctx.every(seconds, fn)` / `ctx.after(seconds, fn)`. Never `setTimeout`.
- **Debug output:** `console.log(...)` / `console.warn` / `console.error` print to stderr and to the
  dev tools console. Remove them when done.

## Dev tools (dev engine builds only)

Press **F1** (gamepad Back+Start) in a dev build of the engine (`build/dev`, `make run-debug`)
for the built-in tools: FPS/frame/script-call overlay, entity inspector (filter by tag, edit
pos/rotation/velocity), console (current error + `console.*` log), pause/step one fixed frame,
and a physics view of collider outlines. `--devtools` opens them at start. The engine bundled in
the SDK is a release build and has no dev tools; nothing in a game can depend on them.

## Rules

- **Deterministic:** randomness only via `ctx.random` (seeded by `config.seed`);
  `Math.random` is disabled. Time only via `ctx.time` / `ctx.dt` (fixed step).
- **State:** per-scene in `state: () => ({...})`, read via `ctx.state`. Data that
  crosses scenes goes through `ctx.go("scene", params)`. Persisted data: `ctx.save`.
  No module-level mutable variables.
- **Entities are data:** prefabs return objects; no classes, no inheritance.
- **No `any`, no type casts (`as const` is fine), no `@ts-ignore`.** If the types reject it, fix the
  declaration in `types.ts` or the code — not the checker.
- Units: pixels, seconds, degrees. Origin top-left, +y down.
- A runtime error pauses the game and reports file:line; it never crashes the engine.

## Tests

Write a `simulate(game, { scene, seconds, inputs, clicks })` test for each behavior you
add. Inputs are action → frame numbers, clicks are `"document#elementId"` → frame numbers, so
runs are reproducible. Assert outcomes (`expect.scene`, `expect.count`, `expect.noErrors`,
`expect.shown("hud")`, `expect.text("hud", "score").equals("3")`), not internals.

- Every exported function in `test/main.test.ts` is one test; `npm test` (= `alpha test`)
  prints `PASS name` / `FAIL name` + the error with `src/...ts:line:col`, exit code 1 on any failure.
- Each `simulate` is a fresh game: seeded random, empty in-memory save, 60 steps per second
  (`seconds: 3` = 180 frames, frames numbered from 0), no window, no sound. Same options => same result.
- Pick inputs from the physics, not by guessing: one flap every 40 frames roughly holds
  altitude; every 20 frames climbs off the top of the screen in ~1.4 s.
- Runtime errors (game or tests) point at the `.ts` source through the bundle's source map.
