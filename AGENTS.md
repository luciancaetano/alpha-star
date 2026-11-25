# alpha-star — 2D engine for AI agents

C++20 core (SDL3 window/render, Box2D v3 physics, internal ECS) driven by
QuickJS-ng scripts that AI agents write for vibecoder users.

## Hard limits (enforced by `tools/check_limits.py`, run before every commit)

| Rule | Limit |
|---|---|
| Lines per function (body, non-blank) | **40** |
| Lines per file (.cpp / .hpp) | **300** |
| Cyclomatic complexity per function | **10** |
| Parameters per function | **10** |
| CRAP score per function (`tools/crapper.py`) | **< 10** |
| Duplication similarity (`tools/dryer.py`) | **< 0.8** |

- Every `.cpp` in `src/` has a matching `.hpp` (except `main.cpp`).
- Headers declare; implementations live in `.cpp`. Only trivial one-line
  accessors, `constexpr` and templates may live in a header.
- Exceeding a limit means split the function/file. Never raise the limit.

## Layout

```
src/<module>/   engine code; one module per folder (core, ecs, physics, render, platform, script, ui, devtools, mcp)
tests/<module>/ doctest tests mirroring src/
alpha-sdk/      npm package: script API types + `alpha` CLI (init/check/export/run/test/mcp) + bundled engines
                + templates/ (`alpha init`; mirrored from examples/ by `npm run templates`)
examples/       five small games (flappy, platformer, topdown, puzzle, shmup): agent reference material
docs/guide/     docs site sources; `cd alpha-sdk && npm run docs` -> docs/site/ (API reference from alpha.d.ts)
tools/          harness levers (limits, crapper, dryer, mutator, check_shipping)
docs/           GLOSSARY.md, LOG.md
cmake/          dependency + warnings config
```

## Commands

```
make build-debug | build-prod | run-debug | debug   # debug = run under VS/lldb/gdb (GAME=path/to/pack)
make flappy                                         # build engine + examples/flappy, then play
                                                    # examples/platformer: sheets, Tiled map, camera
make sdk-engine | sdk-engine-linux                  # stage release engine into alpha-sdk/engine/
make gate                                           # full Windows gate (tools/gate.py, resource-guarded)
make test                                           # tests only (ctest --preset dev under resource_guard)
make linux-gate                                     # full gate in the Linux container: run on the dedicated Linux PC, not this one
```

**Test only on the OS you are on:** on Windows run only the Windows build/tests/gate (never `make linux-gate`
or the Linux container); on Linux run only the Linux ones. Cross-OS runs waste time and tokens.

```
```

```
cmake --preset dev                 # configure (MSVC, Ninja-free VS generator)
cmake --build --preset dev         # build
ctest --preset dev                 # tests
python tools/check_limits.py       # size/complexity/pairing gate
python tools/crapper.py [cov.xml]  # CRAP per function (Cobertura coverage)
python tools/dryer.py              # duplication report
python tools/mutator.py            # mutation testing over src/
```

Game UI is RmlUi (`src/ui/`, `.rml`/`.rcss` in `assets.ui`); Dear ImGui is engine dev tools only.
Text is made at the output resolution (`core::PixelGrid` from SDL's logical presentation): world text in
`render::Renderer::add_text` (atlases per output size, `render::FontCache`), UI text through `ui::FontEngine`
(wraps RmlUi's FreeType engine; BMFont/built-in via `ui::BitmapText`). Fonts load once as `core::FontSource`
(`platform/fonts`: TTF/OTF, BMFont `.fnt`, built-in 8x8 from `tools/make_default_font.py`). Never draw text
rasterized at logical size and scaled. Pack notices: `alpha-sdk/notices/*.txt` from `tools/make_notices.py`
(`--check` verifies them against `build/dev/_deps`); rerun it when a dependency changes.
Physics lives in `src/physics/` (Box2D v3: `physics_world` bodies/contacts/pre-solve one-way,
`queries` rays/overlaps, `joints`); script glue in `script/physics_bindings`, `joint_bindings`
and the embedded `physics.js` / `tween.js` modules. Texture filtering is per image
(`core::Image::smooth`, from `pixelMode`); never set a global scale mode.
Lighting/post effects: `render::Compositor` (render targets + add/modulate blends + vertex colors on SDL_Renderer,
no shaders: SDL 3.2 cannot run custom shaders in SDL_Renderer); data in `core/postfx` + `core/lighting`, render-only.
Input order: dev tools → game UI (`ui::consumes`) → game actions; what the UI consumed never drives actions.
Audio: `core::Mixer` is stereo with volume buses; music is a `core::Stream` (`platform/music_stream`,
stb_vorbis / dr_mp3 in `alpha_codecs`) decoded in `mix()` on the main thread; streams hold SDL audio
streams, so `main` stops them before `SDL_Quit`. Saves/settings/locales are the embedded `alpha:prefs`
and `alpha:i18n` modules (files through `prefsRead`/`prefsWrite`: atomic + `.bak`); `alpha:audio` builds
`ctx.audio`. Never put data-shape rules for these in runtime.js.
Files: every file read/write/list in `src/` goes through `platform/vfs` (SDL_IOStream / SDL filesystem;
whole-file reads closed at once, decoders read from memory, atomic writes, `in_pack(root, name)` for
assets; RmlUi reads through `ui/PackFiles`). `tools/check_file_access.py` (in the gate) fails on any direct
file API (`std::ifstream`, `fopen`, `IMG_Load(path)`, `SDL_LoadFile`, `FT_New_Face`, `fs::exists`, ...)
outside `src/platform/vfs.cpp`. App lifecycle: `core::AppLifecycle` (pure) <- `platform::app_event`
(SDL events) <- `script::AppEvents` in the loop: focus lost / minimized (`config.pauseOnFocusLoss`) and
background pause steps + audio, resume resets the fixed-step clock, hooks `ctx.onPause/onResume/onLowMemory`.
Builds and gates: `CMAKE_BUILD_PARALLEL_LEVEL=8` (the Linux gate also runs with `--cpus 8`); one build at a time.
**Never freeze the user's PC** (a runaway fuzz test once did): `gate.py`, `linux_gate.py`, `stage_engine.py` and
`bench.py` put themselves under `tools/resource_guard.py` (Windows Job Object: 8 GB for the whole tree, 50% CPU hard cap,
below-normal priority, kill-on-close so nothing survives its parent). Run anything else heavy as
`python tools/resource_guard.py -- <command>`; never call ctest/test binaries/bench/fuzz directly. ctest kills a test after
900 s. Every native allocation sized by script data has a hard cap with an agent-facing error (no unbounded reserve/resize
from a script number or string length).
Caps live in `core/limits.hpp` (plus `kMaxParticles`, `Timers::kMinSeconds`, `script::kStackLimitKb`).
Performance and robustness levers (step 13; see `docs/PERF.md`):
- `python tools/bench.py [--only NAME] [--md]`: scenarios in `bench/*.js` on the release engine
  (`--frames 720 --hidden --perf out.json`), ms/frame p50/p95/max per section. Not in the gate.
- Fuzz: `tests/script/fuzz_test.cpp` + `tests/fixtures/fuzz/fuzz_game.js` (3 seeds in the gate);
  longer: `ALPHA_FUZZ_SEEDS=50 ALPHA_FUZZ_STEPS=200 python tools/resource_guard.py -- ctest --preset dev -R fuzz`
  (`ALPHA_FUZZ_ONLY=<regex>`, `ALPHA_FUZZ_TRACE=1` to narrow a finding).
- Soak: `tests/script/perf_script_test.cpp` (1200 steps in the gate; `ALPHA_SOAK_STEPS=216000` = 1 h, under the guard).
- Profiler: dev tools "Profiler" window (F12), MCP `game_state.perf`, `--perf PATH` on `--frames` runs.
- Many entities moved per frame: `ctx.positions(tag)` (bulk), not `entity.pos` per entity.

Dev tools (Dear ImGui, F12 or gamepad Back+Start; `--devtools` opens them at start) build only with `ALPHA_DEVTOOLS`:
never put game-facing features in `src/devtools/`.

### Build flavors (`cmake/Flavors.cmake`)

**Every dev/debug/agent-only feature lands behind its `ALPHA_*` flag and is absent from shipping.**
Test it with `#if ALPHA_X` (never `#ifdef`): each flag is always defined as 0 or 1. Strings in
embedded JS (comments included) end up in the binary too.

| Flag | Gates | dev | release | shipping |
|---|---|---|---|---|
| `ALPHA_DEVTOOLS` | Dear ImGui dev tools (`src/devtools/`, F12, `--devtools`) | ON | ON | OFF |
| `ALPHA_MCP` | agent control channel `src/mcp/` (`--control PORT`, `--hidden`) driven by `alpha mcp` | ON | ON | OFF |
| `ALPHA_DEBUG_TOOLS` | CLI `--frames --screenshot --tap --tap-every --devtools`, `--test` runner, `alpha/test` + its natives, diagnostic logs | ON | ON | OFF |
| `ALPHA_SHIPPING` | forces the three OFF; IPO/LTO, stripped / no PDB, Windows GUI subsystem (no console), asserts off; unknown flags ignored; logs only to `<pref dir>/log.txt` | OFF | OFF | ON |

```
cmake --preset shipping && cmake --build --preset shipping   # player engine
python tools/check_shipping.py                               # fails on any dev/debug/agent marker
python tools/stage_engine.py                                 # stage release + shipping into alpha-sdk/engine/
```
`alpha run`/`alpha test`/`alpha mcp` use the release engine; `alpha export --standalone` copies the shipping one
unmodified and ships `game.jsbc` (bytecode from the release engine's `--compile`, stamped `core::BuildStamp`;
a pack dir runs it before main.js) instead of main.js; `check_shipping.py --pack DIR` verifies such a pack
(`--signed` after `alpha export --sign`, which runs `$ALPHA_SIGN_CMD` on the pack's engine copy only).
Android APKs ship `game.jsbc` too. Release: `python tools/pack_release.py --install-test` packs
`@alpha-star/sdk` + `@alpha-star/engine-<target>` (exact-version optionalDependencies; the SDK resolves
the platform package, then `alpha-sdk/engine/`) into `dist/release/` and never publishes.

`python tools/gate.py` (preset dev; `--preset harness` = fast core-only) runs build + tests + all checks. A change is done only when it passes.

## Workflow

### First make it work
- **Laziness:** one decision lives in one place; ship the smallest edit that solves it.
- **Foundations:** data shape before logic; do first what helps every later phase;
  if a concurrent writer would matter, do not share state.
- **TDD:** write the failing test first, see it fail for the expected reason,
  make the smallest change that passes. If an honest test is impractical, write none.
- **Redesign from first principles:** don't bolt requirements onto old design;
  design as if it had been there from the start, deliver in pieces.
- **Attack the premise:** when two fixes sharing one premise fail the same gate,
  write the premise in one sentence, find which actors hold the imbalance,
  remove that assignment before any further fix.
- **Subtract before you add.**
- **Outcome-oriented:** aim at a checkable end state; mid-way breaks only if planned and undoable.
- **Build the lever:** if work isn't obvious at a glance, build a tool; do one unit by hand first.

### Architecture
- Model the domain so illegal states are impossible; keep `docs/GLOSSARY.md` current.
- Boundary discipline: validate where data enters (script API, files, MCP); trust inside;
  business logic in pure functions.
- Types: make illegal values unwritable; parse outside data once; never silence the
  compiler (warnings are errors; no blanket `#pragma warning(disable)`/NOLINT).
- Operations are idempotent (rerun or crash-midway ends in the same state).
- Separate before serializing shared state; if shared, one writer enforced by structure.

### Then make it right
- Fix root causes: reproduce, fix the cause; a guard that hides a crash is not a fix.
- Sequence work into small verifiable units; finish and check one before the next.
- Test behavior, not implementation: call code as its user does. A test that would
  pass if every function returned nothing gets deleted.
- **Crapper:** every function CRAP < 10 — split it, test it, or both.
- **Dryer:** eliminate duplication ranked ≥ 0.8.
- **Mutator:** cover the uncovered, kill survivors, delete dead code.
- Refactoring: pin behavior, change structure, prove behavior unchanged.

### Practice
- **Docs follow features:** every change that creates or alters a feature (script API, config,
  CLI, asset option) updates, in the same change, the docs (`docs/guide/`, `alpha-sdk/README.md`,
  `alpha.d.ts` comments, `docs/GLOSSARY.md`) **and** the LLM docs in `alpha-sdk/llm/` (keep each
  file's `> summary:` line accurate; add a file for a new feature area). Games pull those through
  the `alpha-harness` skill (`skills/alpha-harness/alpha-harness.md`; skills live in
  `skills/<name>/<name>.md`, versioned here).
- **Git:** work on `main`; never create a branch (or worktree branch) unless the user explicitly asks for one
  (do not infer it from tooling, workflows or conventions).
- **Keep a log:** append every action and its reason to `docs/LOG.md`; report it on completion.
- **Guard the context window:** keep the main thread small; send bulk work to subagents and keep summaries.

## Agent-facing API principles (the product)
- Small, predictable script API; one way to do each thing.
- Errors are written for the agent that will fix them (what failed, what was available).
- A script error pauses the game; it never crashes the engine.
- Deterministic simulation (fixed step, seeded RNG) so agent tests are not flaky.
- Effects are data: particles only through a spawn spec's `emitter` (plus `entity.emit(n)`), layers
  only through `config.layers` + `layer:`; both simulate on the fixed step with the game's seed.
