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
| 61 | Linux port setup: `tools/linux/Containerfile` (Ubuntu 24.04, GCC, SDL X11/Wayland/ALSA/Pulse deps, gcovr, lizard) + `tools/linux_gate.py` (podman/docker, repo at /src, build in a named volume) | Reproducible Linux build without touching the host; WSL had no compiler and sudo needs a password |
| 62 | Podman default container network had no route out (apt timed out) → `--network=host` for build and run | Root cause: container network, not the mirrors |
| 63 | All third-party deps `SYSTEM` (FetchContent), `ALPHA_COVERAGE` (gcov flags via `alpha_set_warnings`), `coverage.py` uses gcovr on Linux, `built_exe` finds any exe per OS | Gate tools run on both OSes; only our code is held to -Werror |
| 64 | SDK CLI `alpha check|export|run` replaces `alpha-build`; export evaluates the game in Node with an `alpha` shim to read `game.assets`, copies declared files into the pack, refuses missing files and paths leaving the project | User: one CLI that builds, packs assets and exports; engine only gets the pack path |
| 65 | Engine accepts a pack directory (`<dir>/main.js`, save in `<dir>/save.json`) | `alpha_star <pack>` |
| 66 | Engines bundled in the SDK per target (`alpha-sdk/engine/<platform>-<arch>/`, git-ignored, staged by `tools/stage_engine.py [--linux]`); `alpha run` resolves --engine, $ALPHA_STAR, bundled, PATH; `alpha export --standalone [--target]` copies the engine into the pack; engine with no game arg runs the `main.js` next to it | User: SDK carries Windows/Linux binaries like Electron |
| 67 | Static MSVC runtime in Debug broke Microsoft coverage (empty coverage.xml → CRAP 0%) → static only in Release | Root cause found by bisecting the one Windows change; shipped exe still needs no VC++ redist |
| 68 | Verified on Windows: staged win32-x64 engine (4 MB), `alpha run` with bundled engine, standalone Flappy export runs fused | End-to-end proof |
| 69 | Not done yet: Linux compile/gate (first container build still running), linux-x64 engine not staged | Recorded; next step |
| 70 | `linux_gate.py` runs containers with `--init` | Root cause of the hung container: `xvfb-run` as PID 1 never received Xvfb's SIGUSR1 ready signal, so the command never started |
| 71 | GCC fixes in tests: explicit `image` initializer in `ShapeSpec` aggregates (`-Wmissing-field-initializers`), `<cmath>`/`<algorithm>` in tessellate_test | MSVC accepted them; GCC with -Werror did not. Engine sources compiled clean |
| 72 | Box2D target: `-Wno-error=maybe-uninitialized` on GCC only | Box2D hardcodes -Werror; GCC 13 -O2 false positive in its own distance.c broke the Linux Release build. Third-party, still visible as a warning |
| 73 | Linux gate GATE PASS (63 tests, gcovr coverage, CRAP, dryer, limits); `stage_engine.py --linux` → `alpha-sdk/engine/linux-x64/alpha_star` (5.8 MB); Flappy dist runs 60 frames headless under xvfb + screenshot | Linux port done |
| 74 | `.github/workflows/ci.yml`: Windows + ubuntu-24.04 gate, SDK `npm ci && npm test`, macOS build+ctest with `continue-on-error` | CI mirrors the local gate; macOS not yet verified |
| 75 | Script call budget 250 ms via `JS_SetInterruptHandler`, deadline set per `Engine::call`/`import` | `while(true){}` pauses with a readable error instead of freezing the engine |
| 76 | QuickJS memory limit 256 MB; OOM errors prefixed with the limit | Allocation bomb pauses, engine survives |
| 77 | Saves in the per-user data dir: `Host(SavePathFor)`, path resolved from `config.title` when the runtime first reads the save; `main.cpp` uses `SDL_GetPrefPath("alpha-star", title)` | Game dir may be read-only/shared; title only known after boot starts, so a function instead of a fixed path |
| 78 | Host tests split: helpers in `tests/script/host_fixture.{hpp,cpp}`, limits/save tests in `host_limits_test.cpp` | host_test.cpp would exceed 300 lines |
| 79 | core: `SourceMap` (source map v3, base64 VLQ, nearest segment at/before the column), `decode_base64`, `inline_source_map`, `rewrite_locations`; pure, unit-tested first (7 failing tests seen) | Errors must name the `.ts` the agent wrote, not `main.js` |
| 80 | `Host` imports the user bundle via `import_bundle` (remembers path + inline map, `script/bundle_map`) and maps every error (pause overlay, stderr, test failures) | One place turns bundle locations into source locations |
| 81 | Game instances: runtime state moved from module globals into a `createInstance(id, save)` closure; native state keyed by id in `Instances` (context opaque); natives act on the active instance, the runtime selects it around each call (`within`) | Redesign: `simulate()` needs isolated games in one JS context (the game object can't cross contexts); alternative of a second Host can't receive the test's game value |
| 82 | `alpha/test` `simulate`: fresh instance per run, fixed 1/60, `holdActions` per frame, in-memory save, `stepBudget` per step (call budget per game step, not per test), snapshot (scene, `tagCounts`, error) then destroy; readable `expect` failures (`expected scene "over" after 180 frames, got "play"`) | Same options => same result; no state or save leaks between runs; no instance lifetime tied to test objects |
| 83 | Test mode `alpha_star --test <file>` (no SDL init, no window/audio): `Host::test` calls each exported function, PASS/FAIL lines, exit 0/1; 8 Host-level tests in `tests/script/simulate_test.cpp` written first and seen failing | Agents run tests headlessly with the shipped engine |
| 84 | SDK `alpha test [--entry test/main.test.ts]` → type-check + bundle to `build/test/<name>.js`, run engine `--test` (same engine lookup as `run`); `lib/test.mjs` + node tests; flappy `npm test` | One CLI for agent tests |
| 85 | Root cause of `workspace/...` source paths: `exportPack` bundled into a temp staging dir, so map sources were relative to it → `bundle(entry, outfile, writeTo)` computes the map for the final location and writes bytes to staging (node test first) | Error paths read `src/scenes/play.ts:27:35` |
| 86 | Flappy test `pipesSpawnOnSchedule` fixed: flapping every 20 frames climbs ~57 px per flap and leaves the top at ~1.4 s (game over, 0 pipes); now one flap per 40 frames (holds altitude) and also asserts scene "play" | The test was wrong, not the engine (probed with simulate at 0.5 s steps) |
| 87 | Verified: `npx alpha test` in flappy 3/3 PASS with the staged engine; a deliberate throw in play.ts shows `src/scenes/play.ts:27:35` on stderr and the overlay (reverted) | End-to-end proof |
| 88 | GCC `-Wsign-conversion` in `main.cpp` test count (`? 0 : 1` int into size_t) → unsigned literals; Linux gate GATE PASS (78 tests), Windows GATE PASS, SDK `npm test` 14/14 | MSVC accepted it; GCC -Werror did not |
| 89 | User decision: Dear ImGui = built-in engine dev tools only (not game UI, which will be RmlUi), never exposed to scripts, compiled out of shipping builds | Scope of this step |
| 90 | Dear ImGui v1.92.9 (FetchContent, tag verified with `git ls-remote`), `alpha_imgui` static target in `cmake/Dependencies.cmake`: core + `imgui_impl_sdl3` + `imgui_impl_sdlrenderer3`, SYSTEM includes, linked to SDL3; option `ALPHA_DEVTOOLS` (OFF by default, ON in preset dev, OFF in release) | ImGui has no CMake build; release fetches/compiles/links none of it |
| 91 | core `LogRing` (newest 200 lines, level log/warn/error), shared per JS context in `Instances::log`; runtime defines `console.log/warn/error` → native `log(level, text)` → stderr `[level] text` + ring; Host logs every pausing error too | Script console for the dev tools; also useful on stderr in shipped engines |
| 92 | Read-only Host accessors for tools: `log()`, `scene()` (runtime `sceneName`), `world()`, `tags()`, `last_call_ms()` (= the last step's script call, recorded by `Engine::call`; scene()/frame() calls do not overwrite it) | Tools never reach into Host internals; each accessor tested in `tests/script/console_test.cpp` |
| 93 | World: `entities()` (living, oldest first; `find_all` now filters it — dryer flagged the copy), `Collider` component kept at spawn, `colliders()` | Inspector listing and physics view need them |
| 94 | `src/devtools/`: pure `TimeControl` (pause / one step per request), `routing` (`is_toggle` F1 or Back+Start, `reaches_game` drops F1 and what `io.WantCaptureMouse/Keyboard` claims), `inspector` (`list_entities` with tag substring filter, `apply` edits, `collider_shapes`), `snapshot` (DevSnapshot); thin ImGui `panels` + `DevTools` (context, event feed, decorate frame, draw with logical presentation disabled so ImGui uses window pixels) | Data gathering pure and unit-tested; drawing tested headlessly (dummy video driver + software renderer: windows really draw pixels) |
| 95 | All tests written first and seen failing against stubs (log ring, time control, inspector, routing, snapshot, DevTools, console/Host accessors, World) | TDD |
| 96 | main.cpp: `Tools` = DevTools in dev builds, a no-op struct otherwise (no #ifdef in the loop); events pass `tools.handle` before game input; steps go through the time control; game frame → physics view → ImGui → screenshot → present; `--devtools` flag; parse split (`set_option`) and `steps_for_frame` kept for CCN <= 10 | One loop for both builds |
| 97 | Problems: SDL `TEXT_INPUT` read through `event.key` in routing (union misuse) → key check only for key events; Flappy console window offscreen on a 432 px wide window → console anchored to the bottom strip | Found in review / screenshot |
| 98 | SDK: `declare var console` (log/warn/error) in `alpha.d.ts` (lib ES2023 has no console); fixture uses it (failing first: "Cannot find name 'console'") | Agents can type-check console.log |
| 99 | Verified: dev build `--devtools --frames 200 --screenshot` shows overlay (60 fps, script call 0.26/250 ms, entities 2, scene play), time controls, entity list (solid, bird), console; release build: ImGui not fetched, 0 "imgui" strings in the exe, Flappy runs; staged win32-x64 engine (release, no dev tools) runs Flappy + `alpha test` 3/3 | End-to-end proof |
| 100 | Gates: Windows GATE PASS (102 tests), Linux GATE PASS (102 tests; ImGui SYSTEM, our devtools code clean under -Werror), SDK `npm test` 14/14; `stage_engine.py` restaged win32-x64 from the release preset — staged engine has NO dev tools (ALPHA_DEVTOOLS OFF) | Done criteria |
