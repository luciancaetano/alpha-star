# Action log

## 2026-11-24 — harness bootstrap

| # | Action | Reason |
|---|---|---|
| 1 | Inspected empty repo + toolchain (CMake 4.3, VS 18 MSVC 19.51, Python 3.13) | Know the ground before choosing tools |
| 2 | `CLAUDE.md` with hard limits + workflow rules | One place the agent reads the rules |
| 3 | `tools/harness_config.py` holds every limit/path | One decision lives in one place |
| 4 | CMake: root, `cmake/Warnings.cmake` (/W4 /WX), `cmake/Dependencies.cmake` (FetchContent, pinned tags) | Reproducible deps; never silence compiler |
| 5 | Presets `harness` (core+tests, no runtime deps) and `dev` (SDL3, Box2D, QuickJS-ng, EnTT) | Fast gate for the inner loop |
| 6 | `alpha_core` + `version` as first unit, version passed from CMake via `ALPHA_VERSION` | Smallest real unit; version defined once |
| 7 | `tools/check_limits.py` (fn ≤40 lines, file ≤300, CCN ≤10, params ≤5, .cpp↔.hpp pairing, no bodies in headers) | Enforce size rules mechanically |
| 8 | `tools/crapper.py`, `tools/dryer.py`, `tools/mutator.py`, `tools/coverage.py`, `tools/gate.py` | Build the levers the rules require |
| 9 | PostToolUse hook in `.claude/settings.json` runs `check_limits` after edits | Agent sees violations immediately |
| 10 | Gate run 1 failed: doctest 2.4.11 vs CMake 4 → bumped to 2.4.12 | Root cause: removed CMake <3.5 compat |
| 11 | ctest passed with 0 tests → `noTestsAction: error` | A passing gate with no tests is dishonest |
| 12 | doctest fwd-declares `std::ostream`, breaks new MSVC STL → `DOCTEST_CONFIG_USE_STD_HEADERS` | Root-cause fix, not a warning disable |
| 13 | `main` CRAP 12 → split `fail()` helper; excluded composition root from crapper | Entry point verified by smoke run, not unit test |
| 14 | Added real coverage (Microsoft.CodeCoverage.Console → Cobertura) | CRAP without coverage is worst-case guessing |
| 15 | Negative test: planted 48-line fn, 7-param fn, unpaired .cpp, 0.99 duplicate → all caught | Prove the gate fails when it should |
| 16 | Mutator on `version.cpp`: 0/1 survived | Test kills the mutant |
| 17 | `dev` preset: fetched + built SDL3 3.2.10, Box2D 3.1.0, QuickJS-ng 0.10.1, EnTT 3.15.0; smoke run opened window, printed `alpha-star 0.1.0`, exit 0 | Prove runtime deps link before engine work |
| 18 | `release` preset + `Makefile` (build-debug, build-prod, run-debug, debug) | User request |
| 19 | Removed hardcoded VS paths: `tools/toolchain.py` resolves VS via vswhere, falls back to lldb/gdb, finds exe per generator; `coverage.py` uses it | Hardcoded `D:/...` only worked on one PC |
| 20 | `examples/flappy/game.js` + `game.test.js`: reference Flappy script defining the target script API (scenes, entity specs, tag collisions, actions, seeded random, save, headless `simulate`) | Design the API from a real agent-written game before implementing it in C++ |
| 21 | `alpha-sdk/`: npm package with script API types (`types/alpha.d.ts`), shared `tsconfig.game.json`, and `alpha-build` (tsc type-check → esbuild bundle to one ES2023 module, `alpha`/`alpha/*` external, errors as `file:line:col`) | Separable SDK: agents get typed API + one-file output the engine loads |
| 22 | Scene params not inferred: a contextually-typed `ctx` makes inference circular in TS → scene/action/sound names are checked, params are annotated in `enter()` | Tried reverse-mapped + `NoInfer`; both left `ctx` as implicit any |
| 23 | Moved package from `tools/alpha-sdk` to root `alpha-sdk/` | User request |
| 24 | API v2: game declares names once (`interface { actions; sounds; tags; scenes }`), `define<G>()` returns typed `scene`/`game`; scene params, tags, scene completeness and start scene now checked; typed `state()` per scene; `ctx.every/after` timers; `Entity.alive` | Explicit type-first declaration breaks the inference cycle from entry 22; data shape before logic |
| 25 | `examples/flappy` rewritten in TS, multi-file (`types`, `alpha`, `tuning`, `prefabs/*`, `scenes/*`, `ui`, `main`) built by `alpha-build` to `dist/main.js`; planted 4 errors (wrong params, unknown tag, missing scene, start with params) → all rejected | Prove the types catch what agents get wrong |
| 26 | `examples/flappy/AGENTS.md`: layout, commands, how-to-add recipes, determinism/state rules for agents | User request; agents need one page to edit the game correctly |
| 27 | core: `Vec2`, `Color` (`#rgb/#rrggbb/#rrggbbaa`), `Random` (PCG32), `FixedStep`, `Timers` (every/after), `Actions` + input names, `TagRegistry`, `EntitySpec`/`DrawItem`/`TextItem` data | Pure, dependency-free pieces the script API maps 1:1 onto |
| 28 | physics: Box2D v3 wrapper in px/deg (50 px/m), hits + sensor enters with owner ids | One place converts units; scripts never see meters |
| 29 | Box2D 3.1 contact events are opt-in (`enableContactEvents` default false) → enabled per shape | Root cause of "no hits" test failure |
| 30 | `b2MakeRot` is approximate (30° → 30.03°) → rotation tests use tolerance | Library property, not a bug to hide |
| 31 | ecs: `World` on EnTT (deferred destroy, creation-order find, errors name the entity) | Deterministic queries despite id reuse |
| 32 | render: pure tessellation + text layout; `Renderer` on SDL with letterboxed logical size, debug-font text, `save_bmp`; tested on SDL software renderer reading real pixels | Honest render tests without a window |
| 33 | platform: SDL events → `InputSet`, taps shorter than a step still register | Input never lost between fixed steps |
| 34 | demo: native Flappy on the layer + `main --frames/--screenshot/--flap-every`; screenshots checked (play + game over) | End-to-end proof; headless lever for agents |
| 35 | dryer flagged `set_position`/`set_rotation` and two index lookups → `World::place`, `core::index_of` | Remove duplication, not raise threshold |
| 36 | check_limits never recognized templates (lizard names omit `template`) → looks at lines above; negative test still flags non-template header bodies | Tool now matches the documented rule |
| 37 | Harness preset failed CRAP on engine files it doesn't build → crapper `--only src/core` for harness; gate default preset → dev | Score only what the preset compiles; full gate sees everything |
| 38 | Removed `src/demo` (native C++ Flappy) | User: end developers only download the binary and write JS; no game code belongs in the engine |
| 39 | script: QuickJS-ng `Engine` (modules from embedded sources or files, promise/job handling, errors = message + stack) | One place owns the JS runtime lifecycle |
| 40 | Runtime in JS (`runtime.js`, `alpha.js`, `alpha_test.js`) embedded via CMake `configure_file`; C++ exposes flat `__native` (26 natives) | Scene/ctx logic stays readable JS; C++ surface stays small and validated |
| 41 | `spec_parser`: JS entity spec → `EntitySpec`, errors name the field (`spawn: shape.color must be a color like "#73bf2e"`) | Validate at the boundary, trust inside |
| 42 | `native_guard`: every C++ exception → JS TypeError; `Host` turns any JS error into pause + on-screen overlay | A script error never crashes the engine |
| 43 | `main`: `alpha_star <game.js> [--frames --screenshot --tap --tap-every]`; save file `<game.js>.save.json` | Binary runs any compiled game; headless lever for agents |
| 44 | Host tests run real JS games (boot, input, timers, collisions→scene+params, save across runs, entity proxy, 7 agent-facing errors, 3 load failures) | Behavior tested as the script author uses it |
| 45 | examples/flappy (TS → dist/main.js) runs in the binary: title, play, game over, save; screenshots checked | End-to-end proof on the real example |
| 46 | dryer → `xy_args`, `new_array` template, merged `randomRange`/`randomInt` into one `random(min,max,integer)` | Remove duplication instead of raising threshold |
| 47 | Removed `Body.type` from SDK types (runtime doesn't provide it); documented text width = size per char | Types must not promise more than the engine does |
| 48 | Not done: audio output (play validates only), `alpha/test` simulate (throws), TS source maps for error lines | Recorded so the next unit starts from facts |
| 49 | User: engine works like LÖVE — one executable that takes a built JS module and runs it | Confirms `alpha_star game.js`; assets resolve relative to the module |
| 50 | core: `synthesize` (sine/square/triangle/saw/noise, freq slide, attack/release, note sequences, fixed-seed noise) + `Mixer` (volume, pitch, loop, stop, master, clamp) | Pure, testable audio; deterministic procedural sounds |
| 51 | Audio pushed from the main thread into `SDL_AudioStream` (50 ms target), no callback | No state shared with the audio thread |
| 52 | SDL_image release-3.2.4 (3.4 needs SDL 3.4), stb backend, no submodules | PNG/JPG/BMP without system libraries |
| 53 | platform: `load_wav` (→ mono f32 48 kHz), `load_image` (→ RGBA8), `AudioOut`; tested with a hand-written WAV, PNG round-trip, dummy audio driver | Honest tests without real hardware |
| 54 | render: sprites as cached textures; geometry batches split at images so z order holds | Images and shapes interleave correctly |
| 55 | script: assets preloaded at boot (`loadSound`/`synthSound`/`loadImage`), `ctx.audio.play/tone/stopAll/volume`, voices, `shape.image`; errors name the asset and list what exists; script error stops all audio | Agent-facing API, validated at the boundary |
| 56 | SDK types: `GameTypes.images`, `Synth`/`Note`/`SoundAsset`, `PlayOptions`, `Voice`, sprite shape; build test finds the error line instead of hardcoding it | Types match the engine exactly |
| 57 | Flappy: WAV paths that never existed → procedural flap/point/crash sounds | Preload now really loads; missing files fail boot |
| 58 | dryer → `load_file<Table, Decode, Kind>` native template | One body for every file asset kind |
| 59 | Not done: OGG/MP3 (WAV only), streaming music (whole file in memory), audio not audible-verified (device opens; samples tested) | Recorded for the next unit |
| 60 | Makefile: `make flappy` (build engine + compile example + play), `flappy-build`; `run-debug`/`debug` open `GAME` (default flappy); `tools/run.py` forwards game + engine args (also under the debugger) | User request; run targets were calling the binary without a game |
