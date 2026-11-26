# alpha-star 0.3.0 — release notes

Status: **prepared, not published.** Publishing is a manual step for the maintainer (commands at
the end). Changes since 0.2.0 (which was never published): [CHANGELOG.md](../CHANGELOG.md#030---2026-11-26).

## Install (once published)

Needs Node.js 20+. Engines ship for Windows x64 and Linux x64.

```
npx @alpha-star/sdk@0.3.0 init my-game --template platformer
cd my-game
npm install
npm test        # headless game tests
npm start       # play (F12 dev tools)
npx alpha export --standalone --out build/game     # game folder for players
```

Add it to an existing project with `npm install --save-dev @alpha-star/sdk`; check with
`npx alpha --version`. Full guide: [docs/guide/getting-started.md](guide/getting-started.md#install).

## Packages

| Package | Contents | Installs on |
|---|---|---|
| `@alpha-star/sdk` | CLI, types, templates, Android template, agent skills, LLM docs, third-party notices | any OS (Node 20+) |
| `@alpha-star/engine-win32-x64` | `alpha_star.exe` (release) + `glslang.exe` + `shipping/alpha_star.exe` | Windows x64 |
| `@alpha-star/engine-linux-x64` | `alpha_star` (release) + `glslang` + `shipping/alpha_star` | Linux x64 |
| `@alpha-star/engine-android-arm64` | `debug/` + `shipping/` `libmain.so` + `libSDL3.so` (prebuilt APK engine) | any OS (APKs build anywhere) |

Every binary is built by GitHub Actions (`.github/workflows/engines.yml`), with one game.jsbc key
(`ALPHA_JSBC_SECRET`) for all engines, so a game exported on Windows or Linux runs on the Android
engine. `alpha export --target android-arm64` needs only a JDK and the Android SDK Platform +
Build-Tools; `--engine-src` still builds the engine from source.

The engine packages are exact-version `optionalDependencies` of the SDK (the esbuild pattern):
npm installs only the desktop one whose `os`/`cpu` match (the Android one always). Installing with `--omit=optional` leaves the SDK
without an engine. Elsewhere (macOS, Linux arm64...) the SDK still type-checks and exports packs;
pass `--engine <path>` or set `ALPHA_STAR` to an engine built from source.

## Verify a release (maintainer, before publishing)

```
python tools/gate.py                               # Windows gate: GATE PASS
python tools/stage_engine.py                       # release + shipping into alpha-sdk/engine/win32-x64
cd alpha-sdk && npm test && cd ..
python tools/pack_release.py --install-test --require win32-x64
# on the Linux machine:
make linux-gate && python tools/stage_engine.py --linux
python tools/pack_release.py --install-test --require linux-x64
```

Each engine tarball is packed on its own OS. The SDK tarball is the same from either machine.

## Known limitations

- **Linux engine package** must be built, gated and packed on the Linux machine; the SDK names
  `@alpha-star/engine-linux-x64@0.3.0`, so publish it before (or with) the SDK.
- **macOS/iOS:** unverified, no engine package; Metal shaders planned (`core::ShaderFormat`).
- **Custom shaders** need SDL's GPU renderer on Vulkan; without it the effect is skipped with one
  warning. Post effects cover the world, not the RmlUi UI.
- **Android:** verified on one phone (Galaxy A17, Android 16); release signing via keystore env
  vars not tested with a real key; MCP is off on Android.
- **`game.jsbc` encryption** stops casual extraction, not a determined attacker (the key ships in
  the engine).
- **Signing:** hooks only (`--sign` + `$ALPHA_SIGN_CMD`); no real certificate tested.
- **CI** has never run on GitHub.
- npm scope `@alpha-star` must be owned by the publisher before the first publish.

## Publish

Automatic: pushing this version to `main` runs `.github/workflows/release.yml` (builds both engines,
tests, packs, publishes to npm, tags `v0.3.0`, creates the GitHub release); see BUILDING.md
"Continuous integration". Needs the repository secrets `NPM_TOKEN` and `ALPHA_JSBC_SECRET` (a long random string, never
changed: it is the game.jsbc key of every engine) and the `npm` environment.

Manual fallback (after approval):

```
git tag -a v0.3.0 -m "alpha-star 0.3.0"
git push origin main v0.3.0
# engine packages first (the SDK's optionalDependencies point at them), each from dist/release/
npm publish dist/release/alpha-star-engine-win32-x64-0.3.0.tgz --access public
npm publish dist/release/alpha-star-engine-linux-x64-0.3.0.tgz --access public   # packed on Linux
npm publish dist/release/alpha-star-sdk-0.3.0.tgz --access public
```
