# AGENTS.md — Arena (alpha-star game)

Top-down arena shooter: walk with the stick / WASD, hold fire (shots auto-aim at the nearest
enemy), bomb clears the enemies around you. A second gamepad joins as player 2.

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
npx alpha run -- --frames 120 --hidden --screenshot s.bmp   # headless screenshot
npx alpha mcp               # MCP server: start, drive and observe the game as an agent
node tools/make-assets.mjs  # regenerate the placeholder art (committed; CC0, made here)
```

A change is done only when `npm run export` prints `exported dist (...)` and `npm test` passes.

## Layout

| File | Holds |
|---|---|
| `src/types.ts` | Every name: actions, axes, sounds, tags, sheets, UI ids, scenes. Edit first. |
| `src/tuning.ts` | Speeds, hit points, spawn rate, bomb radius. Change feel here. |
| `src/prefabs/actors.ts` | Hero (sheet sprite, `data.player`), enemy, bullet (sensor), walls. |
| `src/prefabs/effects.ts` | One-shot particle blast, floating "+1" world text. |
| `src/scenes/play.ts` | Players, steering, auto-aim, bombs (`ctx.queryCircle`), chasing, damage. |
| `src/scenes/over.ts` | Score and best score (`ctx.save`). |
| `assets/ui/` | HUD and game-over screen (RmlUi, built-in font). |

## How things work here

- **Players:** `ctx.input.player(n)` reads one player's actions and axes; player 1 has the
  keyboard, the mouse and the first pad. Heroes carry `data.player`. Tests drive player 2 with
  `inputs: [p1, p2]`.
- **Hits:** bullets are sensors -> `ctx.onEnter("bullet", "enemy", ...)`; enemies are solid ->
  `ctx.onHit("hero", "enemy", ...)`. Several contacts can arrive in one step: check `e.alive`
  first. `ctx.destroy(e)` takes effect at once for reads (`e.pos` throws after it), so read
  positions before destroying.
- **Area query:** `ctx.queryCircle(center, radius, { tags: ["enemy"] })` returns what overlaps.
- **Effects are data:** `emitter: { burst, oneShot: true, ... }` on a spec; the entity goes away
  with its last particle. `ctx.tween(label, { to: { y, alpha: 0 }, seconds, onDone })`.
- **Feel:** `ctx.camera.shake(px, s)`, `ctx.input.rumble(strength, s, player)`, sounds panned by
  x (`ctx.audio.play("shot", { pan })`).
- **Determinism:** `ctx.random` (seeded by `config.seed`), never `Math.random`. Same inputs, same run.

F12 opens the dev tools while the game runs (not in exported standalone games).
