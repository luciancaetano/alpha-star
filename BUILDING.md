# Building the engine

## Requirements

- **Windows:** Git, Node.js 20+, Python 3, CMake 3.28+, Visual Studio 2022 with the C++20 desktop tools.
- Network access on the first configure: CMake fetches SDL3, Box2D, QuickJS-ng, RmlUi and the other
  native dependencies into `build/<preset>/_deps`.

Limit the build to 8 parallel jobs and run one build at a time:

```powershell
$env:CMAKE_BUILD_PARALLEL_LEVEL = "8"
```

## Presets

| Preset | Configuration | Use |
|---|---|---|
| `dev` | Debug | Engine development, tests, coverage. |
| `harness` | Debug | Fast core-only check, no runtime dependencies. |
| `release` | Release | Engine used by `alpha run`, `alpha test` and `alpha mcp` (dev tools included). |
| `shipping` | Release | Player engine: no dev tools, MCP or test runner; LTO, stripped. |

```powershell
cmake --preset dev
cmake --build --preset dev        # -> build/dev/src/Debug/alpha_star.exe
```

Same for the other presets (`cmake --preset release && cmake --build --preset release`).

## Update the SDK engine (needed for `npm run start` in the examples)

`alpha run` / `alpha test` (and so `npm run start` / `npm test` in a game folder) do **not** use
`build/`: they run the engine copy staged in `alpha-sdk/engine/<platform>-<arch>/`. After any engine
change, restage it or games will fail against the old engine:

```powershell
python tools/stage_engine.py      # or: make sdk-engine
make deploy-engine                # all of it: Windows + Linux engines (container), templates, skills, docs site, notices check, SDK tests
```

It builds `release` and `shipping` and copies them into `alpha-sdk/engine/`, under the resource guard.

## Run a game with a local build

```powershell
cd examples/flappy
npm install
npx alpha export                  # writes dist/
cd ../..
build/dev/src/Debug/alpha_star.exe examples/flappy/dist
```

`make flappy` does the build, export and run in one step.

## Android

Setting up the JDK, the Android SDK and its packages to build APKs: [BUILDING-android.md](BUILDING-android.md).

## Tests and the gate

```powershell
python -m pip install -r tools/requirements.txt
make test                         # C++ tests (ctest --preset dev, guarded)
python tools/gate.py              # full gate: build, tests, coverage, limits, CRAP, duplication
```

Never run ctest, test binaries, benchmarks or fuzzers directly: wrap them as
`python tools/resource_guard.py -- <command>` so they cannot freeze the machine.
The code rules are in [AGENTS.md](AGENTS.md).

## Continuous integration (GitHub Actions)

- `.github/workflows/ci.yml` (every push and PR): the full gate on Windows and Linux, staged
  engines, SDK tests, examples, standalone export check, release tarballs.
- `.github/workflows/engines.yml` (push to `main`, or run by hand): builds the Windows debug engine
  (preset `dev`), the Windows and Linux release + shipping engines (`tools/stage_engine.py`), then the
  SDK with both staged into `alpha-sdk/engine/<target>/` (`npm ci`, `npm test`,
  `tools/pack_release.py --install-test`). Artifacts: `engine-win32-x64-debug`, `engine-win32-x64`,
  `engine-linux-x64`, `sdk-release` (the npm tarballs; never published).
- The Linux package list lives in `.github/actions/linux-deps` (mirrors `tools/linux/Containerfile`).

## GitHub Pages showcase

Run `npm --prefix alpha-sdk run docs` to regenerate the landing, guides and API reference.
Edit `alpha-sdk/scripts/landing.mjs` and `landing.css` for the showcase; keep screenshots in
`docs/guide/assets/`. These are actual engine captures, copied by the same generator.
The setup guide source is `docs/guide/getting-started.md`. The existing Pages workflow
publishes `docs/site/` when these sources reach `main`. See `docs/guide/assets/README.md`
for capture commands.
