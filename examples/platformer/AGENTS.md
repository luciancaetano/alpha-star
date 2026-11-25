# AGENTS.md — Platformer (alpha-star game)

You are editing a game for the alpha-star engine. Write TypeScript in `src/`;
`alpha export` type-checks it and bundles it into one JS module (`dist/main.js`)
that the engine runs. The API types live in `../../alpha-sdk/types/alpha.d.ts`;
read that file when unsure, it is the source of truth. `../flappy/AGENTS.md`
covers input, sounds, UI, saves and the MCP tools; this file covers what is
specific to a scrolling game: sprite sheets, a Tiled map and the camera.

## Commands

```
npm install                 # once
npm run check               # type-check only (fast)
npm run export              # type-check + bundle + copy assets -> dist/ (images packed into an atlas)
npm start                   # export, then play (A/D or arrows to move, Space/W/Up to jump, R restarts)
npx alpha run --watch       # every save re-exports and the game hot-reloads
npm test                    # headless simulate() tests (test/main.test.ts)
npx alpha run -- --frames 120 --screenshot s.bmp   # headless screenshot (+ a `render:` stats line)
node tools/make-assets.mjs  # regenerate the placeholder art and the map (committed; CC0, made here)
```

A change is done only when `npm run export` prints `exported dist (...)` and `npm test` passes.

## Layout

| File | Holds |
|---|---|
| `src/types.ts` | Every name: actions, axes, sounds, tags, **sheets -> animation names**, **maps**, UI ids, scenes. Edit first. |
| `src/tuning.ts` | Speeds, jump, gravity, camera lerp/deadzone/shake. Change feel here. |
| `src/prefabs/player.ts`, `coin.ts` | `EntitySpec`s using sheets (`shape: { sheet, animation }`). |
| `src/prefabs/effects.ts` | Landing dust (one-shot `emitter`), the "+1" world text, parallax hills (`layer: "far"`). |
| `src/scenes/play.ts` | Spawns the map, the player at the `player` spawn point, coins at `coin` points; movement, animation choice, landing shake. |
| `src/scenes/won.ts` | All coins collected. |
| `assets/img/*.png` | Sprite sheets (16x16 hero frames: 0 idle, 1-4 run, 5 jump, 6 fall; 8x8 coin spin). |
| `assets/maps/level1.tmj` | Tiled map: tile layers `deco`, `walls` (solid), object layer `spawns`. Tileset `tiles.tsj` -> `../tiles/tiles.png`. |
| `assets/ui/` | HUD and win screen (RmlUi); font in `assets/fonts` (OFL). |

## How to do things

- **New animation:** add frames to the sheet image, then `animations: { name: { frames, fps?, loop? } }`
  in `main.ts` and the name in `types.ts` `sheets`. `entity.animation = "name"` switches (setting the
  playing one again does nothing); `entity.play("name")` restarts. `entity.flipX` mirrors.
- **Editing the level:** open `assets/maps/level1.tmj` in Tiled (save as JSON, "Tile Layer Format: CSV",
  tilesets as `.tsj`, not infinite). Tiles on `walls` are solid (merged into few static boxes tagged
  `"walls"`); `deco` is drawing only. Add objects to `spawns` with a class/type (`coin`) or a name
  (`player`); scripts read them with `ctx.map("level1").objects("spawns")` (x, y in map pixels).
- **New solid layer:** name it in Tiled, add the name to `tags` in `types.ts`, list it in
  `spawnMap("level1", { collide: [...] })`, react with `ctx.onHit("player", "<layer>", ...)`.
- **Camera:** `ctx.camera.follow(entity, { lerp, deadzone })`, `ctx.camera.bounds([x, y, w, h])`
  (here the map size), `ctx.camera.shake(px, seconds)`, `zoom`, `rotation`, `pos.x/y`.
  `ctx.camera.toWorld(ctx.input.mouse())` turns the pointer into world coordinates. The HUD never moves.
- **Effects:** `config.layers` in `main.ts` (`far` scrolls at 0.2 of the camera, `fx` draws on top);
  `layer: "far"` on a spec puts it there. Dust is `emitter: { burst, oneShot: true, ... }` (its entity
  goes away with the last particle); `entity.emit(n)` bursts on demand. World text:
  `shape: { text, font: "pixel", size, color }`, `entity.text = "..."`, fade with `entity.alpha`.
  `config.window.scaling: "integer"` keeps pixels square (Alt+Enter: fullscreen).
- **Pause on focus loss:** losing focus, minimizing or backgrounding pauses the game (steps + audio);
  scene hooks `ctx.onPause(fn)` / `ctx.onResume(fn)` / `ctx.onLowMemory(fn)`, `ctx.app.paused`;
  `config.pauseOnFocusLoss: false` opts out. Tests: `simulate(game, { appPauses: [[from, to]] })`.
- **Tests:** `expect.animation("player").equals("run")`, `expect.particles().atLeast(n)`, `expect.count("coin")`, held movement with
  `inputs: { axes: { moveX: [[0, 1]] } }` (value held from that frame on). Same inputs, same run.

## F12 dev tools

Press **F12** (gamepad Back+Start) while the game runs (`npm start`, `alpha run`, `--watch`): FPS
overlay, entity inspector, console, pause/step, physics view. `--devtools` opens them at start.
Exported standalone games have none; nothing in a game can depend on them.

Errors are written for you: a misspelled sheet, animation, map or layer names what exists.
