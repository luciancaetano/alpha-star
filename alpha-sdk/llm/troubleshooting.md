# Troubleshooting

> summary: Common errors an agent hits (type errors, boot/asset errors, runtime script errors, engine/SDK version mismatches, MCP and export failures) and what each means; read when a check, test, run or export fails.

## Where errors show up

- `alpha check` / MCP `alpha_check`: `file:line:col message` type errors. Misspelled names (tags, actions, sounds, animations, ui ids) are compile errors: add the name to `src/types.ts` or fix the spelling.
- Runtime script errors pause the game (never crash it), source-mapped: overlay, terminal, MCP `game_state` / `game_logs`. In tests, `run.expect.noErrors()` reports them.
- Messages say what failed and what exists (`have: ...`, `actions: ...`): read the list and pick a valid name.

## Common messages

| Message (abridged) | Meaning / fix |
|---|---|
| `Math.random is disabled ... use ctx.random` | Use `ctx.random.range/int/pick` (determinism). |
| `find("x"): no living entity has this tag` | Nothing has it now; use `ctx.findAll("x")[0]` and handle absence, or spawn it first. |
| `...("x"): not in config.actions; actions: ...` | Add the action to `config.actions` (and `GameTypes.actions`). |
| unknown animation (lists the sheet's animations) | Use a name from `assets.sheets.<s>.animations`. |
| `start "x" is not a scene; scenes: ...` / `scenes.k must be scene("k", {...})` | `start` and the `scenes` map must match `GameTypes.scenes`; each value from `scene("k", ...)`. |
| `spawnMap("x"): not in assets.maps; have: ...` | Declare the map in `assets.maps` and `GameTypes.maps`. |
| `...pixelMode is not an image option; use filter: ...` | Images/sheets/maps take `filter: "nearest" \| "linear" \| "sharp"`; `pixelMode` is only for config and fonts. |
| `entity N: data must stay JSON-safe ...` | Only numbers, strings, booleans, null, arrays, plain objects in `entity.data`. |
| `postfx.set("x"): not added in this scene; ... use postfx.add` | Call `ctx.postfx.add` first (each scene starts with none). |
| `postfx "crt": unknown param "amount"; have: strength` | Use the listed params. |
| `camera.bounds must be [x, y, width, height] or null` | Pass a 4-number array. |
| `settings.uiScale must be a number in [0.5, 3]` | Clamp the value. |
| `config.lighting must be { ambient: "#rrggbb" }` | Fix the config shape. |
| `config.layers.x must be { z: number, parallax?: number }` | Fix the layer entry. |
| `key: file "x" not found (paths are relative to ...)` | Asset path wrong; paths are relative to the project and must stay inside it. |
| `... references "x", not found inside the project` | A map/.rml/.rcss links a missing file. |
| `...: save tilesets as JSON (.tsj) in Tiled` | Re-save the tileset as .tsj. |
| `main module must export default game({...})` | `src/main.ts` must `export default game({...})`. |
| a throw when reading `entity.body` / `.text` / `.tint` or calling `.emit` | The entity was spawned without `body` / text shape / shape or emitter / `emitter`. |

## Engine vs SDK version mismatches

The engine binaries must match the SDK (`alpha --version` = bundled engines' version).
- Symptom: an option that `alpha.d.ts` declares is rejected or ignored at runtime, or a documented feature is missing: the engine in use is older than the types. In an alpha-star checkout the SDK uses `alpha-sdk/engine/<platform>-<arch>/`, which goes stale after engine changes: restage with `python tools/stage_engine.py`. Outside the repo, reinstall so `@alpha-star/engine-<platform>` has the SDK's exact version. Also check `--engine` / `$ALPHA_STAR` overrides.
- `the SDK's release engine ... and its shipping engine are different builds; restage both with tools/stage_engine.py`: export refuses mixed builds.
- `no release engine for this machine to compile game.jsbc; stage both flavors ...` or `no shipping engine for target "x"; SDK has: ...`: stage engines or use `--no-bytecode` / another target.
- Standalone game stops at start naming two versions: `game.jsbc` and the engine come from different exports; re-export both together.

## MCP and runs

- `no game is running: call game_start first`.
- `no control channel from <engine> on port N ... must be a release build with ALPHA_MCP`: the engine is a shipping build or stale; restage.
- `game_reload` errors change nothing; fix and reload again.
- Window size/title changes need a restart (`game_stop` + `game_start`), not a reload.

## Flaky or surprising behavior

- Tests differ between runs: something non-deterministic (Date, real time, iteration over external state). Use `ctx.random`, `ctx.time`, `ctx.dt`.
- Handler acts twice / on a dead entity: several contacts per step; check `e.alive` or a `data` flag.
- UI text reset or buttons dead after `ctx.go`: documents reload hidden and handlers drop on scene change; redo show/set/on in `enter`.
- Pixel art blurry or uneven: `config.window.scaling: "integer"`, `filter: "nearest"` or `"sharp"`.
- World text shows `?`: only ASCII + Latin-1 draw in world text; use RmlUi with a font that has the glyphs.
