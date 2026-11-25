# Performance

`python tools/bench.py [--only NAME] [--md]` runs each `bench/*.js` scenario headlessly on the
release engine (`--frames 720 --hidden --perf out.json`, fixed seed, one fixed step per frame) and
reports ms per frame, p50 / p95 / max over the last 600 frames, per section (`core::Section`).
"render" = building the draw list + `Renderer::draw` (SDL_RenderGeometry submission; present and
vsync wait are not counted). Draws / verts are the last frame's world geometry; JS MB = JS heap in
use at the end. Measured on the user's Windows 11 PC (Direct3D 11 renderer) under
`tools/resource_guard.py` (50% CPU hard cap, below-normal priority): absolute numbers are
pessimistic; compare rows of the same table.

| Scenario | What |
|---|---|
| sprites_proxy | 2000 image sprites, `e.pos.x = ...; e.pos.y = ...` per entity per frame (10000 exceed the 250 ms call budget) |
| sprites_batch | 10000 image sprites moved with `ctx.positions("s")` + `write()` |
| physics | 2000 dynamic circle bodies falling into a box |
| particles | 4 emitters x 5000 = 20000 live particles (the world cap) |
| tilemap | 500 x 500 tile layer (8 px tiles), camera scrolling |
| ui | RmlUi document, 200 elements whose text changes every frame |

## Baseline (before step 13's fixes)

| scenario | total | script | physics | particles | ui | render | JS MB |
|---|---|---|---|---|---|---|---|
| sprites_proxy (2000) | 47.65/50.76/61.44 | 47.04/50.12/60.88 | 0.00 | 0.00 | 0.01 | 0.54/0.71/1.18 | 15.6 |
| sprites_batch (10000) | did not run: spawning/moving hit the 250 ms budget | | | | | | 76.4 |
| sprites_proxy (10000) | did not run: one step > 250 ms | | | | | | |
| tilemap | did not boot: `spawnMap` > 250 ms | | | | | | |
| physics | 1.89/4.68/6.55 | 0.02/0.07/0.19 | 0.10/2.90/4.84 | | | 1.62/2.00/2.62 | 15.6 |
| particles | 1.45/1.92/5.01 | 0.02/0.04/0.06 | | 0.04/0.06/0.11 | | 1.37/1.80/4.81 | 0.4 |
| ui | 4.26/4.86/7.86 | 1.30/1.52/2.18 | | | 2.95/3.45/6.62 | 0.00 | 0.4 |

## After

| scenario | total | script | timers | physics | particles | animation | ui | render | audio | draws | verts | entities | JS MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| particles | 1.47/1.81/5.68 | 0.03/0.04/0.10 | 0.01/0.01/0.03 | 0.00/0.00/0.01 | 0.04/0.06/0.15 | 0.00/0.00/0.01 | 0.00/0.01/0.02 | 1.38/1.71/5.49 | 0.00/0.00/0.02 | 1 | 118482 | 4 | 0.4 |
| physics | 2.11/5.03/7.82 | 0.02/0.09/0.19 | 0.01/0.01/0.02 | 0.13/3.01/5.18 | 0.00/0.00/0.00 | 0.00/0.00/0.00 | 0.00/0.01/0.01 | 1.87/2.27/5.34 | 0.00/0.00/0.01 | 1 | 72018 | 2003 | 0.8 |
| sprites_batch | 7.04/7.69/9.33 | 3.17/3.52/3.91 | 0.01/0.01/0.02 | 0.00/0.00/0.01 | 0.00/0.00/0.00 | 0.00/0.00/0.00 | 0.00/0.01/0.13 | 3.81/4.44/6.09 | 0.00/0.00/0.01 | 1 | 60000 | 10000 | 2.6 |
| sprites_proxy | 11.61/12.23/15.09 | 10.92/11.44/14.35 | 0.00/0.01/0.01 | 0.00/0.00/0.00 | 0.00/0.00/0.00 | 0.00/0.00/0.00 | 0.00/0.01/0.01 | 0.71/0.84/1.33 | 0.00/0.00/0.01 | 1 | 12000 | 2000 | 0.8 |
| tilemap | 0.70/0.90/1.13 | 0.03/0.05/0.08 | 0.01/0.01/0.02 | 0.00/0.00/0.00 | 0.00/0.00/0.00 | 0.00/0.00/0.00 | 0.00/0.01/0.01 | 0.66/0.84/1.08 | 0.00/0.00/0.01 | 1 | 23124 | 1 | 4.4 |
| ui | 4.04/4.73/7.78 | 1.15/1.46/2.10 | 0.00/0.01/0.12 | 0.00/0.00/0.01 | 0.00/0.00/0.00 | 0.00/0.00/0.00 | 2.79/3.37/6.65 | 0.00/0.00/0.01 | 0.00/0.00/0.01 | 0 | 0 | 0 | 0.4 |

## Boot time: main.js vs game.jsbc (step 17a)

Whole process, release engine, `alpha_star <pack> --frames 1 --hidden` (start, window + renderer,
boot the game, one frame, exit), 1 warm-up + 11 runs each, under `tools/resource_guard.py`:

| Game | main.js (bytes) | game.jsbc (bytes) | JS median ms (min-max) | bytecode median ms (min-max) |
|---|---|---|---|---|
| flappy | 33736 | 6458 | 469.4 (463.9-481.6) | 466.3 (463.4-472.0) |
| topdown (largest example) | 35731 | 7561 | 354.6 (351.3-364.9) | 351.2 (348.3-363.1) |

main.js sizes include the inline source map. Bytecode saves ~3 ms (no parse/compile); process
start is dominated by SDL, the window and the renderer. Bytecode is about hiding source, not speed.

## What changed and why (LOG 261-263)

- `World::set_position` / `destroy` looked up children by scanning and sorting the whole world:
  moving n entities was O(n^2) per frame. Now only linked entities are scanned.
  sprites_proxy (2000) script 47.0 -> 13.8 ms p50.
- Entity proxies were object literals of ~40 closures (~7 KB each); now shared prototypes
  (`this.id`), `body` made on first use. JS heap: 10000 sprites 76.4 -> 2.6 MB, 2000 bodies
  15.6 -> 0.8 MB; sprites_proxy script 13.8 -> 10.9 ms.
- Boundary cost, per entity per frame (move x and y): proxies = 6 native calls = 5.5 us
  (10.92 ms / 2000); `ctx.positions` = 2 native calls per frame in total = 0.32 us
  (3.17 ms / 10000, the rest is the JS loop): **17x cheaper**. Hence the batch API; per-entity
  proxies stay for gameplay code touching few entities.
- `spawnMap` cell grouping: one plain loop reusing the current tileset (was forEach + Map lookup
  per cell): a 500 x 500 layer now boots inside the call budget.
- Remaining hot spots, not changed: render submission of large vertex counts (particles 118k
  vertices = 1.4 ms, 10000 sprites = 3.8 ms; one SDL_RenderGeometry call each), RmlUi text
  relayout (200 changing texts = 2.8 ms), Box2D p95 at 2000 bodies while they settle (3 ms).
