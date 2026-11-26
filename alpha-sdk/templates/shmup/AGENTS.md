# AGENTS.md — Starfighter (alpha-star game)

Vertical shoot 'em up: fly with the stick / WASD, hold fire, three lives. Streamed music,
a parallax starfield, collision categories.

You are editing a game for the alpha-star engine. Write TypeScript in `src/`; `alpha export`
type-checks it and bundles it into one JS module (`dist/main.js`) that the engine runs. The API
types live in `node_modules/@alpha-star/sdk/types/alpha.d.ts`: read that file when unsure, it
is the source of truth.

## Commands

```
npm install                 # once
npm run check               # type-check only (fast)
npm test                    # headless simulate() tests (test/main.test.ts)
npm start                   # export, then play
npx alpha run --watch       # every save re-exports and the game hot-reloads
npx alpha mcp               # MCP server: start, drive and observe the game as an agent
node tools/make-assets.mjs  # regenerate the art and the music loop (committed; CC0, made here)
```

A change is done only when `npm run export` prints `exported dist (...)` and `npm test` passes.

## Layout

| File | Holds |
|---|---|
| `src/types.ts` | Every name: actions, axes, sounds, music, images, tags, layers, categories, UI, scenes. |
| `src/tuning.ts` | Speeds, fire rate, enemy waves, star depths. Change feel here. |
| `src/prefabs/actors.ts` | Ship, enemy (kinematic), shot (fast sensor), star, explosion. |
| `src/scenes/play.ts` | Starfield, flying, firing, enemy sway, hits and lives. |
| `src/scenes/over.ts` | Fades the music out, "again" button. |

## How things work here

- **Collision categories:** `config.collision.categories` + `category` / `collidesWith` on bodies.
  Shots only meet enemies, enemies never block each other. Both bodies must accept each other.
- **Many entities:** the 120 stars move through `ctx.positions("star")` (edit `y[i]`, then
  `write()` once) — the fast way for many entities per frame.
- **Layers:** `config.layers` (`stars` far behind with parallax 0.3, `fx` on top) + `layer:` on specs.
- **Audio:** `ctx.audio.music.play("theme", { fadeIn })` streams a file (here a generated WAV);
  `music.stop(seconds)` fades out; `expect.music("theme")` / `null` in tests. Sounds are synthesized
  (`assets.sounds` notes) and panned by x.
- **Hits:** `ctx.onEnter("shot", "enemy")` (sensor), `ctx.onHit("ship", "enemy")` (solid). The
  blink while safe is a yoyo tween on `alpha`; `ctx.input.rumble` shakes the pad.
- **Determinism:** `ctx.random` and `ctx.time`; the same inputs always give the same run.

F12 opens the dev tools while the game runs (not in exported standalone games).
