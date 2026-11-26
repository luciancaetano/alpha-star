# alpha-star 0.2.0 — release notes

Status: **prepared, not published.** No npm package, git tag or GitHub release exists yet;
publishing is a manual step for the maintainer (commands at the end).

alpha-star is a LÖVE-style 2D engine for AI agents: a C++20 engine (SDL3, Box2D v3, QuickJS-ng,
RmlUi) run by TypeScript games, and `@alpha-star/sdk`, the `alpha` CLI that type-checks, bundles,
tests, runs and exports them, plus an MCP server so agents can drive a running game.

## What's in it

- **Engine:** fixed-step deterministic simulation (seeded RNG), ECS, Box2D physics (events,
  queries, joints, one-way, CCD), sprites/sheets/animation, atlas packing, Tiled maps, camera,
  particles, layers/parallax, sharp text (TTF/OTF at output resolution, BMFont, built-in font),
  RmlUi game UI with gamepad navigation, audio (WAV/OGG/MP3, streamed music, buses), saves,
  settings, localization, post effects (bloom, crt, pixelate, tint, vignette) and 2D lighting,
  app lifecycle (pause on focus loss/background), gamepads (axes, rumble, hot-plug), touch.
- **Build flavors:** dev/release engines have dev tools (F12), the MCP control channel and the
  headless test runner; the **shipping** engine has none of them (checked by
  `tools/check_shipping.py`), no console, logs to the player's data folder.
- **SDK (`alpha`):** `init` (templates blank, platformer, topdown, puzzle, shmup), `check`,
  `export` (packs; `--standalone` with the shipping engine and `game.jsbc` bytecode; `--sign`),
  `export --target android-arm64` (Gradle project), `run [--watch]` (hot reload), `test`
  (headless `simulate` + `expect`), `mcp` (MCP server for agents), `--version`.
- **Examples:** flappy, platformer, puzzle, shmup, topdown (each with tests).
- **Docs:** `docs/site/` (guides, API reference generated from `alpha.d.ts`, AGENTS.md for games).

## Packages

| Package | Contents | Installs on |
|---|---|---|
| `@alpha-star/sdk` | CLI, types, templates, Android template, third-party notices | any OS (Node 20+) |
| `@alpha-star/engine-win32-x64` | `alpha_star.exe` (release) + `shipping/alpha_star.exe` | Windows x64 |
| `@alpha-star/engine-linux-x64` | `alpha_star` (release) + `shipping/alpha_star` | Linux x64 |

The engine packages are exact-version `optionalDependencies` of the SDK (the esbuild pattern):
npm installs only the one whose `os`/`cpu` match. The SDK finds its engine in that package, then
in `alpha-sdk/engine/<target>/` (a repo checkout), and `--engine` / `$ALPHA_STAR` override both.

## Install (once published)

```
npx @alpha-star/sdk@0.2.0 init my-game --template platformer
cd my-game
npm install
npm test        # alpha test: headless game tests
npm start       # alpha run: play it (F12 dev tools)
npx alpha export --standalone --out build/game     # Game folder for players
```

On a platform without an engine package (macOS, Linux arm64...) the SDK still type-checks and
exports packs; pass `--engine <path>` or set `ALPHA_STAR` to an engine built from source.

## Verify a release (maintainer, before publishing)

```
python tools/gate.py                               # Windows gate: GATE PASS
python tools/stage_engine.py                       # release + shipping into alpha-sdk/engine/win32-x64
cd alpha-sdk && npm test && cd ..
python tools/pack_release.py --install-test --require win32-x64
make linux-gate && python tools/stage_engine.py --linux   # on the Linux machine, then
python tools/pack_release.py --install-test --require linux-x64   # (there)
```

`pack_release.py` writes `dist/release/*.tgz` and `MANIFEST.txt` and checks one version
everywhere (CMake, package.json, optionalDependencies, CHANGELOG, `alpha_star --version`), the
LICENSE and third-party notices in every tarball, no build junk, size budgets, and engines
byte-identical to the staged ones. Each engine tarball must be packed on its own OS (the Linux
engine is built on Linux).

## Known limitations

- **Linux gate not run on this PC** (user decision): the Linux engine in `alpha-sdk/engine/linux-x64`
  here is stale (staged before bytecode), so `pack_release.py` skips the Linux engine package on
  this PC. It has to be built, gated and packed on the dedicated Linux machine.
- **Android APK never built here:** the Android SDK/NDK/CMake/command-line tools are not installed
  on this PC. The Gradle project is generated and tested; the APK build, the arm64 engine and
  `game.jsbc` loading from APK assets are untested on a device.
- **macOS unverified:** no Mac; CI builds it with `continue-on-error`. No macOS engine package,
  no `.app` bundle, no notarization flow tested (documented commands only).
- **Xbox / consoles:** need the GDK and platform access; not started.
- **Custom shaders:** need SDL 3.4 (`SDL_GPURenderState`); 0.2.0 has built-in effects only. Effects
  cover the world, not the RmlUi UI.
- **Crafted bytecode:** `game.jsbc` is encrypted and authenticated (XChaCha20-Poly1305) with a
  key that ships inside the engine: it stops casual extraction and refuses any changed byte, but a
  determined attacker can extract the key from the engine and decompile or craft bytecode (which
  can crash QuickJS). The pack folder is trusted, exactly like a native game's files.
- **Signing:** hooks only (`--sign` + `$ALPHA_SIGN_CMD`, Android keystore via environment); no
  real certificate was used in testing (a fake Authenticode-shaped signer is), and
  `check_shipping --signed` can strip only PE (Authenticode) signatures.
- **CI** has never run on GitHub (nothing pushed).
- npm scope `@alpha-star` must be owned by the publisher before the first publish.

## Publish (manual, after approval)

```
git tag -a v0.2.0 -m "alpha-star 0.2.0"
git push origin feat/script-api-sdk v0.2.0
# engine packages first (the SDK's optionalDependencies point at them), each from dist/release/
npm publish dist/release/alpha-star-engine-win32-x64-0.2.0.tgz --access public
npm publish dist/release/alpha-star-engine-linux-x64-0.2.0.tgz --access public   # packed on Linux
npm publish dist/release/alpha-star-sdk-0.2.0.tgz --access public
```
