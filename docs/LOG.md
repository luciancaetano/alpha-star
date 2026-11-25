# Action log

Condensed summary of the project's history. The full per-action log (326 entries) is in git: `git show 85b6b64:docs/LOG.md`.

## Decisions by the user (binding)

- Limits are never raised by the agent. Parameter limit raised 5 -> 10 by user decision (RmlUi's `GenerateString` takes 9).
- Dear ImGui = engine dev tools only; RmlUi = game UI; scripts never see ImGui.
- Engine runs a pack: `alpha_star <pack>` (LÖVE-style). SDK carries per-platform engines.
- Linux gate runs on a dedicated Linux PC, not on the dev PC.
- Never freeze the user's PC: heavy commands go through `tools/resource_guard.py`.
- Work on `main` only; no branches unless asked.

## Steps

1. **Harness (11-24).** CMake presets (`harness`, `dev`, `release`, `shipping`); gate with limits, CRAP, dryer, mutator, coverage. Gate proven by planted violations.
2. **Core + script API.** C++20, SDL3, Box2D v3, QuickJS-ng, EnTT. Typed TypeScript API (`define<G>()`), QuickJS host. Script errors pause the game with an overlay; the engine never crashes.
3. **Physics, audio, rendering.** Box2D in px/deg (50 px/m). Stereo mixer with buses; streamed OGG/MP3 decoded on the main thread. Batched rendering with camera culling and atlas packing.
4. **Linux port.** Container gate; GCC warnings fixed at their cause. CI runs Windows and Ubuntu.
5. **Safety.** 250 ms script call budget, 256 MB JS heap, 512 KB stack limit, native caps in `core/limits.hpp`. Fuzzing froze the PC once; the cause was native memory sized by script data, now capped.
6. **Build flavors.** `ALPHA_DEVTOOLS`, `ALPHA_MCP`, `ALPHA_DEBUG_TOOLS`; shipping has none. `check_shipping.py` verifies the binaries by markers.
7. **Agent control.** `alpha-control` loopback protocol (token, one client, 1 MiB line cap). `alpha mcp` exposes 13 tools. Hot reload builds a new VM and swaps it only on success.
8. **UI and input.** RmlUi with gamepad navigation. Input order: dev tools, then UI, then actions. Per-player input with hot-plug slots and rumble.
9. **Assets and maps.** Sprite sheets, animations, Tiled JSON maps, parallax layers, particles, world text via FreeType atlas.
10. **Physics API.** One-way platforms, raycasts and queries, joints, parent/child links, tweens.
11. **Saves, settings, i18n, accessibility.** Atomic writes with `.bak`, versioned saves, `{ zero, one, other }` plurals, reduced motion, high contrast.
12. **File access.** All file I/O goes through `platform/vfs`; `check_file_access.py` enforces it. Pause on focus loss and background with `ctx.onPause`.
13. **Pixel-accurate text.** Text is rasterized at output resolution (`PixelGrid`), never scaled. BMFont and built-in VGA font supported. Third-party notices generated.
14. **Profiler and fuzzing.** Profiler window, `--perf` reports, fuzz harness, leak fix for entity proxies, bulk `ctx.positions` API.
15. **Templates and docs.** `alpha init` with five templates mirrored from the examples. Docs site generated from `alpha.d.ts`.
16. **Shaders and lighting.** Built-in effects (bloom, CRT, pixelate, tint, vignette) on render targets. Custom shaders deferred until an SDL 3.4 bump.
17. **Bytecode export.** `game.jsbc` with a 64-byte header (format, length, FNV-1a checksum, engine/QuickJS/build stamps). Bytecode is not encryption.
18. **Android (exporter only).** Gradle project from the SDK template; touch maps to mouse and virtual buttons. APK not built: no NDK on this PC.
19. **Release 0.2.0 (not published).** Per-platform engine packages (`win32-x64`, `linux-x64`), `pack_release.py`, signing hook `ALPHA_SIGN_CMD`, CI jobs.
20. **Docs and examples.** README (now in English), CONTRIBUTING, CHANGELOG. Flappy Orbit art, audio and pixel-art player (32x32 at 1x/4x).

## Open items

- `linux-x64` engine not restaged since the 0.2.0 work; Linux gate pending on the Linux PC.
- Android: native build and device run not verified (toolchain missing).
- macOS: experimental CI build only.
- Not yet: custom shaders, world text beyond Latin-1, antialiased particles.
- MCP `game_reload` test failed once in a full run (cause not captured); not reproduced since.
- assets: image filtering is now `filter: "nearest" | "linear" | "sharp"` on images, sheets and maps (replaces their `pixelMode`; config.pixelMode stays the default, fonts keep `pixelMode`). `core::Filter` on `core::Image`; "sharp" = sharp bilinear without shaders (SDL 3.2): renderer uploads the image grown up to 4x with nearest (`core/image_filter`, max side 4096) and samples linear. `alpha export` atlases per filter (`alpha-atlas.json` -> `filters.<filter>`, sharp pages <= 1024). Old `pixelMode` on an image errors with the fix.
- window: `config.window.scaling: "crop"` (SDL overscan) and `borderless`, `alwaysOnTop`, `maximized`, `vsync` flags.
- flappy: player uses `filter: "sharp"` at 48/132 px (square pixels, clean when tilted); checked by screenshot.
- AGENTS.md: test only on the OS you are on (no Linux gate from Windows and vice versa).
- Gate: PASS (Windows). SDK pack/atlas tests 20/20, flappy tests 15/15.
- Examples failed on `npm run start`: staged SDK engine predated the `filter` image option; restaged via stage_engine.py. Added BUILDING.md (linked from README) explaining presets and that alpha run uses alpha-sdk/engine, not build/.
- alpha-harness skill (skills/alpha-harness/alpha-harness.md + scripts/harness.mjs): copies alpha-sdk/llm/*.md (new per-feature LLM API docs, shipped in the npm package) into a game's docs/llm and writes a managed AGENTS.md block (docs index + quality limits: params 10, fn 50 lines, file 300 lines, complexity 10, nesting 4). AGENTS.md: features update docs + LLM docs. GitHub Pages workflow (.github/workflows/pages.yml) publishes docs/site; Pages enabled (build_type workflow). SDK README: per-image filter replaces pixelMode.
- alpha skills (alpha-sdk/lib/skills.mjs + test): installs/updates skills/<name>/<name>.md as <agent dir>/<name>/SKILL.md for chosen agents (claude .claude/skills, agents .agents/skills [default pair], cursor, gemini, copilot .github/skills, opencode); interactive pick or --agents; package ships skills/ via prepack. Docs: guide/agents.md, SDK README, llm/overview.md.
- make deploy-engine: sdk-engine + npm ci + templates + skills copy + docs site + notices --check + SDK tests (AGENTS.md, BUILDING.md).
- make deploy-engine also builds/stages the Linux engines (sdk-engine-linux, container).
- BUILDING-android.md: JDK, Android SDK (Studio or cmdline-tools), pinned packages, env vars, APK build/install, troubleshooting; linked from BUILDING.md and README.
