# AGENTS.md — Flappy (alpha-star game)

You are editing a game for the alpha-star engine. Write TypeScript in `src/`;
`alpha-build` type-checks it and bundles it into one JS module (`dist/main.js`)
that the engine runs. The API types live in `../../alpha-sdk/types/alpha.d.ts`;
read that file when unsure, it is the source of truth.

## Commands

```
npm install          # once
npm run build        # type-check + bundle src/main.ts -> dist/main.js
npm run build:test   # same for test/main.test.ts
alpha_star dist/main.js                                   # play
alpha_star dist/main.js --frames 120 --screenshot s.bmp   # headless check
```

A change is done only when `npm run build` prints `built dist/main.js`.
Errors are `file:line:col: error: ...`; nothing is written while any exist.

## Layout (one responsibility per file)

| File | Holds |
|---|---|
| `src/types.ts` | Every name: actions, sounds, tags, scenes + their params. **Edit first.** |
| `src/alpha.ts` | `define<Flappy>()` → typed `scene` / `game`. Never edit. |
| `src/tuning.ts` | Gameplay numbers (sizes, speeds, timings). Change feel here, not in scenes. |
| `src/prefabs/*.ts` | Pure functions returning `EntitySpec` (plain data). |
| `src/scenes/*.ts` | One scene per file: `export default scene("name", {...})`. |
| `src/ui.ts` | Small UI helpers. |
| `src/main.ts` | `game({ config, start, scenes, assets })`. Registers every scene. |
| `test/main.test.ts` | Headless `simulate()` tests. |

## How to add things

- **New input:** add to `actions` in `types.ts`, then map devices in `main.ts` `config.actions`.
- **New sound:** add to `sounds` in `types.ts`, then in `main.ts` `assets.sounds` either a WAV path
  (relative to `dist/main.js`) or a synth spec `{ wave, freq, duration }` / list of notes.
  Play with `ctx.audio.play("name", { volume, pitch, loop })`; one-off procedural sounds with
  `ctx.audio.tone({ wave: "square", freq: [300, 600], duration: 0.1 })`.
- **New image:** add to `images` in `types.ts`, the file (PNG/JPG/BMP) in `main.ts` `assets.images`,
  then `shape: { image: "name", size: [w, h] }`. All assets preload before the first scene.
- **New entity kind:** add its tag to `tags`, write a prefab in `src/prefabs/`, `ctx.spawn(prefab())`.
- **New scene:** add `name: Params | undefined` to `scenes`, create `src/scenes/name.ts`,
  register it in `main.ts` `scenes`. The compiler lists what is missing.
- **Collision:** in `enter`, `ctx.onHit("a", "b", fn)` (solid) or `ctx.onEnter("a", "b", fn)` (sensor).
- **Timed event:** `ctx.every(seconds, fn)` / `ctx.after(seconds, fn)`. Never `setTimeout`.

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

Write a `simulate(game, { scene, seconds, inputs })` test for each behavior you
add. Inputs are action → frame numbers, so runs are reproducible. Assert outcomes
(`expect.scene`, `expect.count`, `expect.noErrors`), not internals.
