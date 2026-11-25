<p align="center">
  <img src="./logo.png" alt="Alpha Star Engine" width="320" />
</p>

# Alpha Star Engine

A native 2D engine for games written in TypeScript and JavaScript, with tools for AI agents to create, test and inspect the game while it runs.

The project combines an engine in **C++20, SDL3, Box2D v3 and QuickJS-ng** with the **`@alpha-star/sdk`**, which provides the typed API and the `alpha` CLI. Games are defined in code: scenes, entities, rules and assets. The SDK type-checks and bundles the scripts; the engine runs the result without Node.js on the player's machine.

The agent workflow is already implemented: an MCP server lets an agent start the game, send inputs, advance the simulation, query entities, capture images and read errors. You can also develop by hand with the same API and CLI, without any mandatory AI service.

> [!NOTE]
> The repository is at version **0.2.0**, under development. The [release notes](docs/RELEASE_0.2.0.md) record the preparation of the packages, not a publication. The quick start below uses the local code and does not depend on the SDK being available on npm.

## Implemented features

| Area | What exists today |
|---|---|
| Gameplay | Scenes with state and lifecycle, entities defined by specs, tags, timers and tweens. Simulation at 60 steps per second and a seeded random generator. |
| 2D rendering | Sprites, sheets and animations, image atlases, camera, layers, parallax and particles. |
| Physics | Box2D v3: bodies, collisions, sensors, queries, raycasts, joints and one-way platforms. |
| Maps | Import of Tiled JSON maps, tilesets and collider generation. |
| UI and text | Game UI in RmlUi (`.rml`/`.rcss`), gamepad navigation, TTF/OTF fonts, BMFont and an embedded font. |
| Input | Actions and axes for keyboard, mouse, gamepads and touch; multiple local players, rumble and hot-plugging of controllers while running. |
| Audio | WAV, OGG and MP3, synthesized sounds, streamed music, volume buses and stereo panning. |
| Persistence | Saves, settings and localization, including plurals. |
| Effects | Bloom, CRT, pixelate, tint, vignette and 2D lighting. These are predefined effects; there is no custom shader API. |
| Development | Hot reload, headless tests, MCP and F12 dev tools: inspector, console, pause, step-by-step advance, physics visualization and profiler. |
| Distribution | Script and asset packs; standalone export with a player engine and bytecode; Android project generation. |

## Getting started from the repository

### Set up the engine on Windows

Requirements: **Git, Node.js 20+, Python, CMake 3.28+ and Visual Studio with C++20 tools**. Native dependencies are fetched by CMake; the first build needs network access.

In PowerShell, from the folder where you want to clone the project:

```powershell
git clone https://github.com/luciancaetano/alpha-star.git
cd alpha-star
$env:CMAKE_BUILD_PARALLEL_LEVEL = "8"
npm --prefix alpha-sdk ci
python tools/stage_engine.py
```

The last command builds and stages the `release` and `shipping` engines in `alpha-sdk/engine/`. It already limits resource usage. Run one build at a time. Rerun it after every engine change: `npm run start` in a game uses this staged copy, not `build/`. Full build instructions: [BUILDING.md](BUILDING.md).

### Play and test an example

From the checkout, run Flappy Orbit:

```powershell
cd examples/flappy
npm install
python ../../tools/resource_guard.py -- node ../../alpha-sdk/bin/alpha.mjs test
python ../../tools/resource_guard.py -- node ../../alpha-sdk/bin/alpha.mjs run --watch
```

`test` runs the game's tests without a window. `run --watch` opens the game and updates the pack when the code changes. Press **F12** or **Back+Start** on a gamepad to open the development tools.

The commands above use `resource_guard.py` to cap the CPU and memory of the processes. Use this protection as well for builds, tests and benchmarks you run manually in this repository.

### Create your own game

From the root of the checkout, create a sibling folder using the local SDK:

```powershell
$sdk = (Resolve-Path alpha-sdk).Path.Replace('\', '/')
node alpha-sdk/bin/alpha.mjs init ../my-game --template platformer --sdk "file:$sdk"
cd ../my-game
npm install
npm run check
```

Available templates: **`blank`, `platformer`, `topdown`, `puzzle` and `shmup`**. They include code, sample assets, tests and an `AGENTS.md` with guidance for working on the game. The `file:` dependency keeps the new project linked to this SDK checkout.

In a game folder with the SDK installed:

| Command | Result |
|---|---|
| `npx alpha check` | Type-checks the game. |
| `npx alpha test` | Runs the tests using `simulate` and `expect`. |
| `npx alpha run --watch` | Bundles, opens the game and watches for changes. |
| `npx alpha export` | Creates a pack in `dist/`, with `main.js` and assets. |
| `npx alpha export --standalone --out build/game` | Creates a folder for distribution with the shipping engine and `game.jsbc`. |
| `npx alpha mcp` | Starts the MCP server over stdio. |

## How a game is organized

The entry point is `src/main.ts`, which exports the game definition: configuration, initial scene, scenes and assets. The templates split this code into small parts:

```text
src/
  types.ts       action names, tags, scenes and assets used by the API
  alpha.ts       scene and game specialized with the game's types
  tuning.ts      speed, duration and other tuning values
  prefabs/       functions that produce entity specs
  scenes/        behavior and state of each scene
  main.ts        game configuration and composition
assets/          images, audio, fonts, maps and UI
test/            game tests
```

Each scene has the hooks `state`, `enter`, `update` and `exit`. The `ctx` context offers operations such as `spawn`, `find`, `input`, `audio`, `ui`, `camera` and `go`. Entities are created from data; a `body` adds physics and an `emitter` adds particles. Entities, handlers, timers and tweens belong to the scene and are removed when the scene changes.

The fixed step and `ctx.random`, controlled by the game's seed, make it possible to reproduce scenarios with the same inputs in tests. Script errors pause execution and are reported with diagnostics; during development, source maps point back to the original TypeScript.

Read [Concepts](docs/guide/concepts.md), [Testing](docs/guide/testing.md) and the [typed API](alpha-sdk/types/alpha.d.ts) for the full contracts.

## Working with AI agents

Configure your MCP client to run `npx alpha mcp` with the **game folder as the working directory**. A development cycle can use:

1. `alpha_check` and `alpha_test` to verify the change.
2. `game_start` to open the game, with or without a visible window.
3. `game_input` and `game_step` to exercise a scenario.
4. `game_state`, `game_screenshot` and `game_logs` to observe the result.
5. `game_reload` to load the next change.

The engine's control channel is local, uses a per-run token, and is not present in the shipping build. See the [MCP configuration and tool list](docs/guide/mcp.md) and the [guide for agents](docs/guide/agents.md).

## Examples to study

| Game | Demonstrates |
|---|---|
| [Flappy Orbit](examples/flappy) | Menu and game scenes, UI, music, settings, localization and saved high score. |
| [Platformer](examples/platformer) | Sprite sheets, Tiled map, camera, platforms, collisions and particles. |
| [Topdown](examples/topdown) | Two players, auto-aim, physics queries, rumble and lighting. |
| [Puzzle](examples/puzzle) | Sokoban with pure rules, undo moves, saved progress and accessibility options. |
| [Shmup](examples/shmup) | Collision categories, batch movement, parallax, bloom and CRT. |

All of them have tests. Flappy is a repository example; the other four are also `alpha init` templates.

## Export and platforms

A **pack** is the folder the engine runs. The common export contains JavaScript and the declared assets. With `--standalone`, the SDK includes the player engine and compiles the game to **`game.jsbc`**, bound to the engine version/build. The player opens the executable in that folder; they do not need to install the SDK or Node.js. Bytecode is not encryption, and pack files should come from a trusted source.

| Platform | Status documented in this version |
|---|---|
| Windows x64 | Release/shipping builds and the standalone export flow validated locally. |
| Linux x64 | Target and engine package implemented; the gate and release preparation must be run on the dedicated Linux machine. |
| Android arm64 | The exporter generates a Gradle project with assets and bytecode; APK build and on-device execution are not yet validated in the release notes. |
| macOS | Experimental build in CI; no engine package or `.app` distribution validated. |
| iOS and consoles | No export flow implemented or documented in this version. |

Android export uses `alpha export --target android-arm64` and requires the Android toolchain to build the APK. Desktop signing via `--sign` depends on an external command configured in `ALPHA_SIGN_CMD`.

See [Exporting](docs/guide/exporting.md), [Android](docs/guide/android.md) and the [version limitations](docs/RELEASE_0.2.0.md#known-limitations). The existing map integration is with **Tiled**; LDtk is not a delivered integration. The current visual effects act on the world, not on the RmlUi UI.

## Developing the engine

How to build each preset, restage the SDK engine and run the gate: [BUILDING.md](BUILDING.md).

| Directory | Responsibility |
|---|---|
| `src/` | C++ engine: core, ECS, physics, rendering, platform, scripting, UI, devtools and MCP. |
| `alpha-sdk/` | CLI, TypeScript types, templates, exporters and SDK tests. |
| `examples/` | Five complete games used as reference. |
| `tests/` | C++ tests organized by module. |
| `docs/guide/` | Guide sources; `docs/site/` contains the generated site. |
| `tools/` | Build, validation, coverage, benchmarks and packaging. |
| `cmake/` | Dependencies, build options and flavors. |

Builds serve different purposes:

| Preset | Use |
|---|---|
| `dev` | Engine development, tests and coverage. |
| `harness` | Fast core validation, without runtime dependencies; does not replace the full gate. |
| `release` | Optimized engine used by `alpha run`, `alpha test` and `alpha mcp`, with development tools. |
| `shipping` | Player engine, without devtools, MCP or test runner, with LTO and stripped symbols. |

From the repository root:

```powershell
$env:CMAKE_BUILD_PARALLEL_LEVEL = "8"
python -m pip install -r tools/requirements.txt
python tools/gate.py
```

The guarded gate runs configuration, build, tests, coverage and the checks for limits, file access, CRAP and duplication. On Windows, coverage collection also requires `Microsoft.CodeCoverage.Console`; see [the coverage instructions](tools/coverage.py). The full rules are in [AGENTS.md](AGENTS.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

To generate the documentation, run `npm --prefix alpha-sdk run docs` and open `docs/site/index.html`. The API reference is generated from `alpha-sdk/types/alpha.d.ts`. To prepare and test the release packages locally, use `python tools/pack_release.py --install-test`; this command does not publish the packages.
