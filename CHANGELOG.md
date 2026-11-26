# Changelog

All notable changes to alpha-star (engine and `@alpha-star/sdk`) are listed here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versions follow
[Semantic Versioning](https://semver.org/) as described in [CONTRIBUTING.md](CONTRIBUTING.md#versions).
Nothing has been published yet; `docs/LOG.md` has every step in detail.

## [Unreleased]

## [0.4.0] - 2026-11-26

### Removed (breaking)
- Tiled maps: `assets.maps`, `ctx.spawnMap`, `ctx.map`, `GameTypes.maps`, `MapObject`, `TileMap`.
  A game that still declares `assets.maps` stops at boot with a message naming `assets.worlds`.

### Added
- LDtk levels (`assets.worlds`, `.ldtk` with embedded or external `.ldtkl` levels): `ctx.world(name)`
  (levels, `levelAt`, entities by iid, neighbours; Free, GridVania and Linear layouts) and
  `ctx.spawnLevel(world, level, opts)`: tile layers keep LDtk flips and opacity (one batched,
  culled entity per layer), IntGrid values become merged static colliders with per-value tags /
  `oneWay` / `sensor`, entity factories receive typed fields (Point, EntityRef, Tile, FilePath,
  enums), animated tiles from tileset custom data `{"frames": [...], "fps": n}`, and the live level
  reads and edits cells (`int`, `tile` with enum tags and custom data, `setInt` rebuilds colliders,
  `setTile` redraws in place, `toCell` / `toWorld`, `entity(iid)`, `destroy()`). `alpha export`
  copies tileset images and external levels. The platformer example and template use LDtk.
- Custom shaders also compile to MSL (Metal): SDL_shadercross, built without DXC, turns glslang's
  SPIR-V into `.msl` (`alpha run` on load, `alpha export` into packs); `core::ShaderFormat::Msl`
  (entry point `main0`), picked on Metal devices. GLSL stays the only shader language; the same
  path is ready for a D3D12/Xbox format (SPIR-V -> HLSL -> platform compiler). Desktop engine
  packages ship `shadercross` next to `glslang`.

## [0.3.0] - 2026-11-26

First version meant for npm. Summary, install and known limitations:
[docs/RELEASE_0.3.0.md](docs/RELEASE_0.3.0.md).

### Added
- Custom shaders: Vulkan GLSL fragment shaders (`assets.shaders`, compiled to SPIR-V by glslang;
  `alpha export` ships `.spv`), run through `ctx.postfx`; Shader Lab example + `shaders` template.
- Multi-language text: HarfBuzz shaping, bidi (SheenBidi), CJK line breaking, font `fallbacks`,
  world text `dir` and `width`, the same pipeline for RmlUi text.
- `ctx.platform`, `ctx.safeArea`, `ctx.camera.view`, UI `class="safe"`, `simulate({ size, platform })`.
- Antialiased particles (`emitter.smooth`).
- Android: real APKs verified on a device, `--variant debug|release|bundle`, `alpha run --target
  android-arm64`, immersive fullscreen, dev tools on shake / Menu key, 16 KB-aligned `.so`.
- Prebuilt Android engine `@alpha-star/engine-android-arm64` (debug + shipping `libmain.so`/`libSDL3.so`,
  `tools/stage_android.py`): `alpha export --target android-arm64` needs no engine source, NDK or
  CMake (`--engine-src` still builds from source). Desktop engine packages now include `glslang`
  (shader games export from an npm install).
- Release pipeline `.github/workflows/release.yml`: every binary built by GitHub Actions with one
  game.jsbc key, published to npm under `@alpha-star/*` and as a GitHub release.
- `alpha skills` installs the agent skills; docs site landing page with example captures.

### Changed
- **Breaking:** `config.window.scaling` replaced by Godot-style `config.window.aspect`
  (`expand` default | `keep_width` | `keep_height` | `keep`) and `config.window.scale`
  (`fractional` | `integer`); the old field fails with the replacement named.
- SDL 3.2 -> 3.4.
- `game.jsbc` is encrypted: XChaCha20-Poly1305 (Monocypher 4.0.2) with a per-engine-build key
  shared by the release (compiles) and shipping (runs) engines, a random nonce per export, the
  header authenticated (format 2, 112 bytes). Decrypted only in memory, wiped after load. Old
  (format 1), tampered or other-key files are refused with a clear error. Stops casual extraction,
  not a determined attacker (the key ships in the engine). `check_shipping --pack` fails on plain
  bytecode (`--absent TEXT` for game strings).

## [0.2.0] - 2026-11-25

Release candidate: everything below is in 0.2.0, prepared but **not published** (no npm package,
no tag yet). Summary and known limitations: [docs/RELEASE_0.2.0.md](docs/RELEASE_0.2.0.md).
Highlights: `alpha init` templates and five examples, docs site, Android export, post effects and
2D lighting, bytecode standalone packs, per-platform engine packages, signing hooks.

### Added

- Per-platform engine packages, like esbuild: `@alpha-star/engine-win32-x64` and
  `@alpha-star/engine-linux-x64` (`os`/`cpu` set; release + shipping engines, LICENSE,
  THIRD_PARTY_NOTICES.txt) are exact-version `optionalDependencies` of `@alpha-star/sdk`, which no
  longer contains engines. The SDK finds the engine in the installed platform package first, then
  `alpha-sdk/engine/<target>/` (repo checkout), and `--engine` / `$ALPHA_STAR` still override.
  `tools/pack_release.py [--install-test]` builds and verifies the tarballs in `dist/release/`
  (one version everywhere, required files, no build junk, size budgets, engines identical to the
  staged ones) and installs them offline into a temp project (init, test, standalone export).
- `alpha export --standalone --sign`: runs `$ALPHA_SIGN_CMD` (`{file}` = the pack's engine copy;
  e.g. signtool) on the exported engine; the staged engine stays unmodified; no certificates are
  created or stored; a failed signature deletes the pack. `check_shipping.py --pack DIR --signed`
  compares the engine with the staged one once the Authenticode signature is removed.
- Android APKs ship `game.jsbc` too (compiled by this machine's release engine; same build id as
  the arm64 engine built from source); `--no-bytecode` keeps `main.js`. Release keystore for the
  Gradle build from `ALPHA_ANDROID_KEYSTORE*` environment variables (debug key otherwise).
- `alpha_star --version` (release/dev engines) prints `alpha-star <version>`.
- Android target: `alpha export --target android-arm64` writes a Gradle project (SDL's Java glue,
  pack in the APK assets, shipping engine built from source, arm64-v8a) and builds a debug-signed
  APK when the Android toolchain is installed; otherwise prints what to install. `config.android`
  (package, icon, versionCode, version). Guide page "Android".
- Touch inputs `Touch`, `TouchLeft`, `TouchRight`; `Back` (Android back button).
- Orientation from the game size; immediate pause when the OS announces background (event watch).
- Post effects `ctx.postfx` (`bloom`, `crt`, `pixelate`, `tint`, `vignette`; scene-scoped, ordered,
  validated params) and 2D lighting (`config.lighting: { ambient }` + spawn `light: { radius, color?, intensity? }`),
  render-only, on every SDL renderer including the software one. Guide page "Shaders and lighting".
  Shmup example: bloom + CRT toggle (C); topdown: lantern and bullet lights. Custom shader code is not
  available yet (needs SDL 3.4).
- Standalone packs ship bytecode: `alpha export --standalone` writes `game.jsbc` (QuickJS bytecode,
  no source or debug info, compiled by the SDK's release engine via `alpha_star --compile`) next to
  the unmodified shipping engine, instead of `main.js` + source map. `--no-bytecode` opts out.
  Engines run only bytecode stamped with their own version and build (clear error otherwise; header
  with length + checksum). `tools/check_shipping.py --pack DIR` checks a standalone pack.

### Changed

- A pack directory with a `game.jsbc` runs it instead of `main.js`.
- CI: the Windows job stages the engines and runs the SDK tests, every example's tests, the
  standalone pack check and the release pack + install test; the Linux job is for the dedicated
  Linux runner; macOS stays build-only and allowed to fail.
- Documented threat model: bytecode integrity is not a security boundary; the pack folder is
  trusted (Exporting guide).

### Added (step 14)
- `alpha init <dir> [--template blank|platformer|topdown|puzzle|shmup] [--sdk <spec>]`: a ready
  project (package.json, tsconfig, src/{types,alpha,tuning,main}.ts, scenes, prefabs, tests,
  AGENTS.md, generated CC0 art, .gitignore). Templates live in `alpha-sdk/templates/`, mirrored
  from the examples by `npm run templates` (checked by the SDK tests).
- `alpha --version`.
- Three new example games: `topdown` (Arena: two players, auto-aim, area-query bomb), `puzzle`
  (Crates: Sokoban with pure rules, undo, saves, English/Portuguese), `shmup` (Starfighter:
  collision categories, `ctx.positions` starfield, streamed music).
- Docs site `docs/site/` generated by `npm run docs` in `alpha-sdk`: getting started, concepts,
  guides, an API reference generated from `alpha.d.ts` with the TypeScript compiler API, and an
  "AGENTS.md for games" page; `--check` (run by the SDK tests) fails when it is stale.
- `@alpha-star/sdk/png` export (the SDK's PNG encoder, used by projects' asset scripts).
- CHANGELOG.md, CONTRIBUTING.md; README getting started and license (MIT).

### Fixed
- `alpha export` failed for any game importing `vec` from "alpha": the exporter's stand-in module
  lacked it. A test now checks the stand-in has every export of the engine module.
- `alpha.d.ts`: the camera's documentation was attached to `Positions`; console docs said F1.

### Changed
- Engine and SDK version 0.2.0 (they move together).

## [0.1.0] - 2026-11-25

The first complete engine and SDK, built in steps (see `docs/ROADMAP.md`).

### Added
- Core: C++20 engine on SDL3, Box2D v3 and QuickJS-ng; internal ECS; fixed step and seeded random;
  script errors pause the game with source-mapped messages; per-user saves; call budget and
  memory caps; Linux port and CI (step 1).
- `simulate()` + `alpha test`: headless deterministic tests, TypeScript source maps (step 2).
- Dear ImGui dev tools (F12, gamepad Back+Start): overlay, inspector, console, pause/step,
  physics view, profiler (steps 3, 12b, 13).
- Render interpolation; low-latency presentation and short audio buffer.
- RmlUi game UI with gamepad navigation (step 4).
- Build flavors via `ALPHA_*` flags: dev, release, shipping (no dev/agent features, LTO,
  stripped); `alpha export --standalone` ships the shipping engine; `check_shipping.py` (step 4b).
- Gamepad axes, triggers, deadzone, up to four players, rumble, hot-plug (step 5).
- MCP server `alpha mcp`: agents check, test, start, drive and observe the game (step 6).
- Hot reload: `alpha run --watch` and MCP `game_reload` (step 7).
- Camera, sprite sheets and animation, atlas packing, Tiled maps (step 8).
- Particles, tint/alpha/blend, layers and parallax, world text, fullscreen and scaling modes (step 9).
- Physics events, raycasts and queries, categories, materials, joints, one-way platforms, CCD;
  tweens, vector math, parent/child, entity data, prefabs; per-asset pixelMode (step 10).
- Audio: OGG/MP3, streamed music, buses, crossfade, pan; save slots and versioning, settings,
  localization, accessibility options (step 11).
- Portable IO through SDL_IOStream (`platform/vfs`); app lifecycle pause/resume (step 12).
- Antialiased shapes (step 12b); sharp text at output resolution, BMFont fonts, built-in font,
  third-party notices in packs (step 12c).
- Benchmarks, `ctx.positions` batch API, boundary fuzzing, soak tests, leak fixes (step 13).
- Examples: `flappy`, `platformer`.
