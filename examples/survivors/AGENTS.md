# AGENTS.md — Candlewick (alpha-star game)

Survivors-like: Sir Wick, a living candle, walks through an endless night bog while weapons fire on
their own. Gems -> level up -> pick 1 of 3 cards (weapons/passives). A maxed weapon + its passive
evolves from a chest (elites and the Moth Queen drop them). The Snuffer arrives at 15:00. Gold is
banked between runs: Candle Shop upgrades and five more heroes (title screen). Each of the six heroes
starts with a signature weapon only they can get (stronger, with its own hit effect).

You are editing a game for the alpha-star engine. Write TypeScript in `src/`; `alpha export`
type-checks it and bundles it into one JS module (`dist/main.js`). The API types live in
`node_modules/@alpha-star/sdk/types/alpha.d.ts`: read that file when unsure, it is the source of truth.

## Commands

```
npm install                 # once
npm run check               # type-check only (fast)
npm test                    # headless simulate() tests (test/main.test.ts)
npm start                   # export, then play
npx alpha run --watch       # every save re-exports and the game hot-reloads
npx alpha mcp               # MCP server (.mcp.json): start, drive and observe the game as an agent
python -I tools/process-art.py   # art/raw (Codex-generated) -> assets/img sheets, icon fonts, UI art
node tools/record-showcase.mjs <dir> 24       # showcase run (V on the title) -> PNG frames, hidden engine (no window)
python -I tools/encode-showcase.py <dir> <out>  # frames -> showcase.mp4 + poster (docs landing video)
```

**Showcase run:** `V` on the title starts `game/demo.ts`: every kind evolved, short cooldowns, a thick and
tough horde, an autopilot, no damage or menus (`state.demo`). It exists for the docs video; tune it there.

A change is done only when `npm run export` prints `exported dist (...)` and `npm test` passes.

## Layout

| File | Holds |
|---|---|
| `src/types.ts` | Every name: actions, sounds, tags, sheets, UI ids, scenes, item ids. Edit first. |
| `src/tuning.ts` | Speeds, HP, XP curve, enemy and hero stats. Change feel here. |
| `src/data/powers.ts` | Every weapon: kind (how it fires), mod (what its hits do), icon, evolution passive, numbers, signature hero. |
| `src/data/items.ts` | Passives (what they do: `stats` in `run.ts`); re-exports the weapons. |
| `src/data/glyphs.ts` | Icon glyph codes, including the recolored ones `tools/process-art.py` makes. |
| `src/data/waves.ts` | Spawns per minute and timed events (swarms, rings, elites, bosses). |
| `src/game/run.ts` | Run state (JSON, survives hot reload) and derived stats (pure). |
| `src/game/offers.ts` | Level-up offers, chests and evolutions (pure, unit-tested). |
| `src/game/weapons.ts`, `areas.ts`, `projectiles.ts` | Firing per kind (shots / areas) and what weapons leave in the world. |
| `src/game/strike.ts` | A weapon hit: crit, execute, knockback, heal, gold, blast; burn/slow/freeze statuses. |
| `src/game/enemies.ts`, `combat.ts` | Horde spawns/chase/contact; damage, death, drops. |
| `src/game/pickups.ts`, `world.ts` | Gems/items/braziers; endless ground and scenery. |
| `src/game/menus.ts` | HUD and the level-up / chest / pause menus. |
| `src/scenes/` | `title` (heroes, shop), `play` (its `SYSTEMS` list is the per-step order), `over`. |
| `assets/ui/` | RmlUi docs; icons are glyphs of `assets/fonts/icons.fnt` / `bigicons.fnt`. |
| `art/raw/` | Source art generated with Codex (magenta background); never edit `assets/img` by hand. |

## How things work here

- **Icons in UI:** `ctx.ui.set` is text only, so item icons are BMFont glyphs: `A-E` weapons,
  `F-J` evolved, `a-g` passives, `$ %` coin/cake, `1-6` heroes, `#`/`-` XP bar, `*` empty slot.
  Newer powers use recolored copies: `glyph(g, h)` = `chr(0x100 + g * 8 + h)` (same rule in the art tool).
- **New weapon:** add its id to `WeaponId`, a row in `powers.ts` (kind + mod pick its behavior; no code)
  and `weapon.<id>.name/evo/desc` in both locales. Non-custom weapons share the growth texts `grow.*`.
- **Extra heroes are recolors** of the drawn ones (`HERO_VARIANTS` in `tools/process-art.py`).
- **Menus pause the world:** play's `update` sets `ctx.frozen = state.mode !== "play"` first; systems with the
  default `when: "running"` then skip while a menu is up, `menuKeys` is `when: "frozen"`, and the `"always"` ones
  (frozen re-sync, world, hud) run every step.
- **Systems:** `scenes/play.ts` `SYSTEMS` runs in order after `update`: tick (time, regen, walk) -> weapons ->
  projectiles -> horde (spawns, statuses, night events) -> chase (`tag: "enemy"`) -> contact -> pickups -> life
  (death) -> menus -> menuKeys -> frozen -> view -> world -> hud. Type one in its own module as `PlaySystem`
  (`game/run.ts`); give object systems a `name` (errors name it). Keep the order: tests compare numbers.
- **Events (`Candle.events`):** `nightEvent { at, boss }` from `game/enemies.ts` -> banner, boss sound and shake
  (`wireBanners` in `game/menus.ts`). Events arrive at the end of the step, so never move anything that draws
  `ctx.random` (kills, drops, offers) behind an event: the RNG order and every tested number would change.
- **Typed tags:** `Candle.tags` maps each tag to its `data` shape (`types.ts`); prefabs return `Spec<"enemy">`,
  functions take `Ent<"enemy">`, so `e.data.*` needs no casts. Use `e.has("boss")`, not `tags.includes`.
- **Queries and `findAll` only return living entities;** a list held while your own code kills can go stale:
  check `alive` inside loops that kill.
- **Languages:** every player-facing word is in `assets/locales/pt.json` + `en.json` (same keys):
  `ctx.t(key)` in code, `data-t="key"` in RML. First launch is Portuguese; the title button toggles.
  Pure rules take a `t` function (`offers.ts`) so tests can pass `(k) => k`.
- **UI art is pre-enlarged x4** (kept from when the engine filtered UI images linearly; UI images now follow `pixelMode`); size UI in dp = game pixels.
- **Camera follows the hero exactly** (`follow`, `lerp: 1`); never round it (the hero shakes). Still
  scenery sits on whole pixels (`world.ts`) so it moves as one with a fractional camera.
- **Determinism:** `ctx.random` only; the world scenery uses a hash of the cell, not random.

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
| [docs/llm/ui.md](docs/llm/ui.md) | Game UI with RmlUi .rml/.rcss documents: declaring docs and element ids, show/hide/set/on, full-screen layout and class="safe", focus and gamepad navigation, pixel-art UI images (config.uiFilter), accessibility hooks and UI tests; read when building HUDs, menus or buttons. |

## Pixel-perfect movement and the camera (render interpolation)

The engine simulates on a fixed step and **draws every entity and the camera interpolated**
between the previous and the current step (smooth on 120/144 Hz screens). Follow these rules or
sprites will shake or flicker:

- **Follow the player with `ctx.camera.follow(hero, { lerp: 1 })`.** Never set `ctx.camera.pos`
  by hand every step, and never round it (`Math.round`): the camera then jumps in whole pixels
  while the hero is drawn interpolated, and the hero shakes ~0.5 px in the walking direction.
  Use `lerp < 1` only if you want a trailing camera (the world then seems to slide when it catches up).
- **Put still things (scenery, props, tiles) on whole pixels** (`Math.round` their spawn position).
  With a fractional camera they then all shift by the same fraction and move as one block.
  Mixing fractional and whole positions makes them wobble against each other.
- **Teleports need nothing special.** Setting `pos` more than a quarter of the view away is drawn
  there at once (children too). A shorter jump is drawn sliding over one step, so when you recycle
  or relocate entities, keep recycled slots fixed to a world cell (`cell mod gridSize`): only
  off-screen slots move, and they move a whole grid.
- **Size everything that fills the screen from `ctx.camera.view`** (ground tiles, scenery grids,
  spawn and despawn distances), never from fixed numbers: the visible world changes with the
  window, the monitor and the phone (`config.window.aspect`). Recompute when the view size changes.
- **`config.size` is the design canvas, not the window resolution.** Art, UI (1dp = 1 design px),
  lights and distances are measured in it; the engine scales it to any screen. Raising it to a
  "bigger resolution" makes everything smaller. Pick it once for the art (e.g. 384x216 for 32 px
  sprites) and leave it.
- **Test sizes headless:** `simulate(game, { size: [1080, 2340] })` and check coverage against the
  run's `ctx.camera.view` (store it in `ctx.state` to assert on it).

## Lessons from building a full game (Candlewick)

### Entities and queries
- **Queries and `findAll` only return living entities.** No `.filter((e) => e.alive)` after them.
  But a list you hold while your own code kills can go stale: check `e.alive` before reading
  `e.pos` (it throws on a dead entity) inside loops that kill, handlers and timers.
- **Lists are stale inside a loop that kills.** If one strike can kill the next target, check
  `t.alive` again on every iteration.
- **Entities without a body are invisible to queries.** Orbiting lanterns, pools and slashes with
  no `body` are not found by `queryCircle`; use `findAll(tag)`.
- **Several contacts arrive in one step.** Guard `onEnter`/`onHit` handlers with `alive`
  (or a `data` flag) before acting.

### Pausing the world (menus, level-up)
- Keep a `mode` in `ctx.state`, skip your systems while a menu is up, and set
  `ctx.frozen = mode !== "play"`: physics (bodies keep their velocity), contact events,
  `ctx.every`/`ctx.after`, tweens, animations, particles and `ctx.time` stop; `update`, UI handlers,
  input, camera and audio keep running. No need to zero velocities. It resets to false on scene enter.
- Timers you keep in `ctx.state` must still count `ctx.dt` only while playing (`ctx.dt` is not 0 when frozen).

### UI (RmlUi)
- **`ctx.ui.set` is text only.** To show item icons in the UI, make a BMFont whose glyphs are the
  icons (`A` = sword, `B` = bow...) and `set` those letters; bars work the same way with repeated
  glyphs (`"####----"`).
- **Size the UI in `dp` = design pixels**, for the `config.size` canvas (a 384x216 game has
  384dp of width). Plain RCSS panels (`background-color` + `border`) are always crisp; avoid
  stretching one big frame image across the screen.
- **Pixel-art UI images** are crisp by default (UI images follow `config.pixelMode`; override with
  `config.uiFilter`). Draw UI art at its real pixel size and give elements that same size (or a
  whole multiple).
- `float` layouts were unreliable; use `display: inline-block` columns for grids.
- Whitespace collapses: separate inline items with margins, not spaces.
- Two `game_click`s in the same step may conflict: step at least one frame between clicks.

### Languages
- **Every player-facing word lives in `assets/locales/*.json`**: `ctx.t(key)` in code, `data-t`
  in RML. Keep the same keys in every file (a quick test that compares the key sets catches gaps).
- **Default language = `config.locale`.** Don't invent "first launch" flags in the save to pick
  a language: they go stale and lock the player into the wrong language.
- Pure rule functions (offers, chests) take a `t` function, so tests can pass `(k) => k` and
  assert on keys.

### Balance and pacing
- **Spawn rate, not batches:** accumulate `rate * dt` and spawn one foe per whole unit; blend
  rate and cap from one minute to the next. Batches every N seconds read as "all at once".
- **Contact damage must add up per touching foe**, with a short invulnerability (~0.3 s). One hit
  at a time from the strongest foe makes standing still immortal.
- Write a test that a hero who never moves dies, and one that pacing never jumps
  (`waveAt(t)` vs `waveAt(t - 1)`).
- Relocate foes left far behind **well past the view corners** (half the view diagonal + margin),
  or they pop in right next to the player.

### Art pipeline (generated images)
- Generate sprites on a flat magenta background; key it out and slice frames at the widest gaps
  (the expected count is known), then downscale with a premultiplied box filter and hard alpha.
- One fixed cell size per sheet; scale every frame of a sheet by the same factor.
- Keep the raw images in the repo and the processing as a script (`tools/process-art.py`):
  never hand-edit the generated sheets.

### Working with the MCP
- Play-test with a scripted bot: `game_input` axis + `press` + `game_step`, `game_screenshot`
  at key moments. Long runs find real crashes (the stale-target bug was found this way).
- To debug movement, step 1 frame at a time with `game_state` and diff positions between frames:
  it separates simulation bugs (positions jump) from drawing bugs (positions fine, screen wrong).
- Put debug-relevant values in `ctx.state` or entity `data`: `game_state` and tests can read them.

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
