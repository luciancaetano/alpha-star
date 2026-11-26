# Building the engine

## Requirements

- **Windows:** Git, Node.js 20+, Python 3, CMake 3.28+, Visual Studio 2022 with the C++20 desktop tools.
- Network access on the first configure: CMake fetches SDL3, Box2D, QuickJS-ng, RmlUi, FreeType,
  HarfBuzz, SheenBidi, SDL_net, wslay, Mbed TLS (`alpha:net`) and the other
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

## Shader compilers (glslang, shadercross)

Custom shaders (`assets.shaders`) are Vulkan GLSL compiled to SPIR-V by
[glslang](https://github.com/KhronosGroup/glslang) (`cmake/Glslang.cmake`, built from source at a pinned tag,
without SPIRV-Tools or the HLSL front end), then from that SPIR-V to MSL (Metal) by
[SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross) (`cmake/Shadercross.cmake`: pinned commit,
SPIRV-Cross from source at its submodule commit, `SDLSHADERCROSS_DXC=OFF`, so no DXC download, no DXIL and
no extra DLLs). The `dev` and `release` builds put both tools next to `alpha_star` (building the engine builds
them); `tools/stage_engine.py` stages them with the release engine for `alpha run` / `alpha export`. The
`shipping`, Android and Apple builds never build them (`-DALPHA_GLSLANG=OFF` / `-DALPHA_SHADERCROSS=OFF`
turn them off anywhere; without shadercross packs carry SPIR-V only).

- `ALPHA_GLSLANG=<path>` / `ALPHA_SHADERCROSS=<path>` point a dev/release engine at other binaries.
- More formats go through `core::ShaderFormat` (engine) and `SHADER_FORMATS` (`alpha-sdk/lib/shaders.mjs`):
  a Direct3D 12 / Xbox format would be SPIR-V -> HLSL (shadercross, no DXC) -> that platform's compiler.

## game.jsbc key

Standalone packs ship `game.jsbc` encrypted with a key of the engine build (`cmake/BytecodeKey.cmake`,
XChaCha20-Poly1305 from [Monocypher](https://monocypher.org) 4.0.2). The key comes from a secret: `-DALPHA_JSBC_SECRET=...`,
else `$ALPHA_JSBC_SECRET`, else `build/jsbc.secret`, made at random by the first configure of a checkout and shared
by every preset there (release encrypts, shipping decrypts; never commit it). Stage the release and shipping engines
from one checkout (`tools/stage_engine.py` does). Deleting `build/jsbc.secret` makes a new key: reconfigure and
restage every engine, and re-export the games. For reproducible release builds on several machines, set the same
`ALPHA_JSBC_SECRET` everywhere. The binary holds the key only masked; the secret never reaches it.

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
  (preset `dev`), the Windows and Linux release + shipping engines (`tools/stage_engine.py`), the
  prebuilt Android engine (`tools/stage_android.py`, NDK + CMake pinned by the Gradle template), then the
  SDK with both staged into `alpha-sdk/engine/<target>/` (`npm ci`, `npm test`,
  `tools/pack_release.py --install-test`). Artifacts: `engine-win32-x64-debug`, `engine-win32-x64`,
  `engine-linux-x64`, `engine-android-arm64`, `sdk-release` (the npm tarballs; this workflow never publishes).
- `.github/workflows/release.yml` (push to `main` that changes `alpha-sdk/package.json`, or run by
  hand): if `v<version>` is not tagged and not on npm, calls `engines.yml` (templates synced before
  packing), publishes `@alpha-star/engine-win32-x64`, `@alpha-star/engine-linux-x64`, `@alpha-star/engine-android-arm64`, then
  `@alpha-star/sdk` with `npm stage publish` over npm trusted publishing (OIDC: no npm token;
  provenance automatic; reruns skip what is already published), then tags
  `v<version>` and creates the GitHub release (CHANGELOG section as notes; npm tarballs, MANIFEST,
  engine zip/tar.gz). To release: bump the version in `alpha-sdk/package.json` (+ optional deps,
  lock), `CMakeLists.txt` and add the `CHANGELOG.md` section, then push to `main`.
  Every published binary comes from these jobs; all of them use the repository secret
  `ALPHA_JSBC_SECRET` as the game.jsbc key, so a pack compiled by the desktop release engine runs
  on the shipping and Android engines (`pack_release.py` checks it). Staged versions are installable
  only after a maintainer approves them on npmjs.com (or `npm stage approve`, which asks for 2FA).
  Setup once: the repository secret `ALPHA_JSBC_SECRET` (a long random string; never change it), an
  environment named `npm`, and on npmjs.com, for each of the four packages, Settings -> Trusted
  publishing -> GitHub Actions with repository `luciancaetano/alpha-star`, workflow `release.yml`,
  environment `npm`. No `NPM_TOKEN`: the workflow sets no `NODE_AUTH_TOKEN` (npm would try it before OIDC).
- The Linux package list lives in `.github/actions/linux-deps` (mirrors `tools/linux/Containerfile`).

## GitHub Pages showcase

Run `npm --prefix alpha-sdk run docs` to regenerate the landing, guides and API reference.
Edit `alpha-sdk/scripts/landing.mjs` and `landing.css` for the showcase; keep screenshots in
`docs/guide/assets/`. These are actual engine captures, copied by the same generator.
The setup guide source is `docs/guide/getting-started.md`. The existing Pages workflow
publishes `docs/site/` when these sources reach `main`. See `docs/guide/assets/README.md`
for capture commands.

The showcase header uses root `logo.png`, copied by the generator and watched by Pages CI.
Hero slides and gallery share the example list in `landing.mjs`; `landing.js` handles
previous/next, direct selection, keyboard and swipe. There is no automatic rotation.
