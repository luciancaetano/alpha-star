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

<!-- alpha-harness:start (generated; edit outside this block) -->

## alpha-star API docs (read on demand)

This game runs on the alpha-star engine. Do not guess the API: before using a feature,
read the matching doc below (they are short). `node_modules/@alpha-star/sdk/types/alpha.d.ts`
is the full typed reference. These files are copied from the SDK by the alpha-harness skill;
rerun it after updating `@alpha-star/sdk` instead of editing them.

| Doc | Read it when |
|---|---|
| [docs/llm/audio.md](docs/llm/audio.md) | Sound assets (files and synthesized notes), playback options, streamed music with fades/crossfades, and volume buses; read when adding sound effects or music. |
| [docs/llm/camera-and-window.md](docs/llm/camera-and-window.md) | ctx.camera (follow, bounds, zoom, shake, toWorld), draw layers with parallax, config.window aspect/scale (Godot-style: fill the screen, ctx.camera.view = visible world) and flags, ctx.window fullscreen, ctx.safeArea and UI class="safe" (phones: immersive fullscreen, cutouts); read when the view must move or the window must look a certain way. |
| [docs/llm/entities.md](docs/llm/entities.md) | EntitySpec fields, spawn/find/destroy, prefabs as plain functions, tags, parent/child, entity data, looks and bulk positions; read when creating or changing game objects. |
| [docs/llm/exporting.md](docs/llm/exporting.md) | alpha export packs, standalone builds with the shipping engine and game.jsbc bytecode, signing, other targets and Android APKs; read when building something a player will run. |
| [docs/llm/input.md](docs/llm/input.md) | Actions and axes (config.actions/axes), device input names, mouse/touch, ctx.platform hints, multiple players, rumble and rebinding; read when wiring controls or reading player input. |
| [docs/llm/install.md](docs/llm/install.md) | Installing @alpha-star/sdk from npm (new game, existing project, global), the per-platform engine packages, checking the version and fixing a missing engine; read when setting up a machine or project for alpha-star. |
| [docs/llm/mcp.md](docs/llm/mcp.md) | The alpha mcp server's tools for agents (check, test, export, start, step, input, state, screenshot, logs, reload) and the play-test loop; read when driving a running game as an AI agent. |
| [docs/llm/overview.md](docs/llm/overview.md) | Project layout, the types.ts/alpha.ts/scenes/main.ts pattern, GameConfig, the scene lifecycle and ctx, and the alpha CLI; read this first before writing any alpha-star game code. |
| [docs/llm/particles-and-effects.md](docs/llm/particles-and-effects.md) | Particle emitters on spawn specs, looks (tint/alpha/blend), 2D lighting (config.lighting + entity light) and full-screen post effects (ctx.postfx); read when adding visual effects. |
| [docs/llm/physics.md](docs/llm/physics.md) | Box2D-backed bodies, colliders, collision categories, contact handlers, raycasts/area queries, forces/impulses and joints; read when anything should move physically, collide or be detected. |
| [docs/llm/saves-and-i18n.md](docs/llm/saves-and-i18n.md) | Persistent save slots with versioning/migration, player settings (volumes, locale, rebinds, accessibility flags) and localization with ctx.t, plurals and data-t; read when persisting progress, adding options menus or translating text. |
| [docs/llm/shaders.md](docs/llm/shaders.md) | Custom full-screen Vulkan GLSL fragment shaders (assets.shaders + ctx.postfx): the GLSL contract (sets, locations, float std140 uniforms), wave and grayscale examples, how alpha run compiles (glslang to SPIR-V, shadercross to MSL) and alpha export packs .spv + .msl, and the Vulkan requirement per platform; read when a built-in post effect is not enough. |
| [docs/llm/sprites-and-images.md](docs/llm/sprites-and-images.md) | Image assets, sprite sheets and animations, flips, texture filtering (filter nearest|linear|sharp, config.pixelMode) and atlas packing; read when drawing images or animated sprites. |
| [docs/llm/testing.md](docs/llm/testing.md) | Headless deterministic tests with alpha/test simulate(), its options (inputs, clicks, saves, settings, appPauses, platform, size) and checks (expect, entities, state); read before writing or debugging test/main.test.ts. |
| [docs/llm/text-and-fonts.md](docs/llm/text-and-fonts.md) | World text shapes, font assets (TTF/OTF, BMFont .fnt, built-in 8x8), any language (fallback fonts, right-to-left `dir`, wrapping `width`, CJK), pixelMode for fonts and font licenses; read when showing text in the world, choosing fonts for world text and UI, or adding a language. |
| [docs/llm/tilemaps.md](docs/llm/tilemaps.md) | Loading LDtk worlds (.ldtk), spawnLevel with IntGrid colliders, entity factories, typed fields, editing cells, multi-level worlds; read when building levels. |
| [docs/llm/troubleshooting.md](docs/llm/troubleshooting.md) | Common errors an agent hits (type errors, boot/asset errors, runtime script errors, engine/SDK version mismatches, MCP and export failures) and what each means; read when a check, test, run or export fails. |
| [docs/llm/tweens-and-timers.md](docs/llm/tweens-and-timers.md) | ctx.tween on entities and plain objects (eases, yoyo, repeat, onDone), ctx.every/ctx.after timers and the vec helpers; read when animating values over time or scheduling work. |
| [docs/llm/ui.md](docs/llm/ui.md) | Game UI with RmlUi .rml/.rcss documents: declaring docs and element ids, show/hide/set/on, full-screen layout and class="safe", focus and gamepad navigation, accessibility hooks and UI tests; read when building HUDs, menus or buttons. |

## Code quality rules

Keep game code small enough for an agent (and a human) to read whole. When a limit is hit, split
the function or move code to a new module in `src/`; never raise the limit.

| Rule | Limit | Source of the value |
|---|---|---|
| Parameters per function | **10** (prefer ≤ 4; pass an options object beyond that) | project rule; ESLint `max-params` default is 3, SonarQube S107 is 7 |
| Lines per function (body, non-blank) | **50** | ESLint `max-lines-per-function` default |
| Lines per file | **300** | ESLint `max-lines` default |
| Cyclomatic complexity per function | **10** | McCabe's original threshold; also SonarQube/NIST recommendation |
| Nesting depth | **4** | ESLint `max-depth` default |

- One feature per file in `src/` (player, enemies, hud, ...); `main.ts` wires them together.
- Tunable numbers live in one place (`tuning.ts`), not scattered literals.
- Every behavior change comes with an `alpha test` test; run `alpha check` and `alpha test` before you finish.
- Keep the simulation deterministic: use the engine's seeded RNG and fixed step, never `Math.random()` or wall-clock time.

<!-- alpha-harness:end -->
