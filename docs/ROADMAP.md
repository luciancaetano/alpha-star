# Production roadmap (multi-agent pipeline)

One subagent per step, strictly sequential (shared files). Each step: TDD, both gates
(Windows `python tools/gate.py`, Linux `python tools/linux_gate.py`) GATE PASS,
`cd alpha-sdk && npm test` green, LOG.md + GLOSSARY.md updated, one commit on
`feat/script-api-sdk`. Never push or publish externally.

Resource rules (user's PC, they may be gaming): `CMAKE_BUILD_PARALLEL_LEVEL=8`, one
build/gate/container at a time, `podman machine start` only for the Linux gate and
`podman machine stop` right after, at most one release build per step.

| # | Step | Status |
|---|---|---|
| 1 | Linux port, CI, call budget, memory limit, per-user saves | done d316e06 |
| 2 | simulate() + `alpha test`, TS source maps | done 26aec1d |
| 3 | Dear ImGui dev tools (F1) | done d17227e |
| - | Render interpolation (frame pacing) | done 69d5ea1 |
| - | Low latency: D3D11 frame queue 1, short audio buffer | done 588c3af (gated in step 4b) |
| 4 | RmlUi game UI (old ctx.ui dropped) + UI gamepad navigation | done 24bce97 |
| 4b | Build flavors via compile flags (#if): ALPHA_DEVTOOLS (exists), ALPHA_MCP, ALPHA_DEBUG_TOOLS (headless CLI flags, --test runner/alpha/test, diagnostics), umbrella ALPHA_SHIPPING (forces all off, LTO, stripped, WIN32 subsystem/no console, asserts off); presets dev/release/shipping; `alpha export --standalone` ships the shipping engine; CI/test asserts the shipping binary has no devtools/MCP/test-runner symbols or strings; AGENTS.md rule: every dev/debug feature lands behind its flag | done (this commit; LOG 114-124) |
| 5 | Gameplay joystick: axes, triggers, deadzone, players, rumble, hot-plug, simulate axes | done (this commit; LOG 125-138) |
| 6 | MCP server (agent drives the game: start, input, state, screenshots, errors) | done (this commit; LOG 139-152) |
| 7 | Hot reload (pack re-export -> running engine reloads) | done (this commit; LOG 153-162) |
| 8 | Camera (follow/zoom/shake/bounds) + sprite sheets & animation + atlas packing + tilemaps (Tiled import) | done (this commit; LOG 163-176) |
| 9 | Particles, tint/alpha/blend, layers/parallax, world text, fullscreen & scaling modes | done (this commit; LOG 177-190) |
| 10 | Physics: end events, raycasts/queries, layers/filters, materials, joints, one-way, CCD; tween/easing, vector math, parent/child, entity data, prefabs; per-asset pixelMode filtering | done (this commit; LOG 191-205) |
| 11 | Audio: OGG/MP3, streaming music, buses, crossfade, pan; save slots/versioning, settings; localization; accessibility | todo |
| 12 | Portable IO (SDL_IOStream everywhere) + app lifecycle (suspend/resume, focus pause) | todo |
| 13 | Performance: benchmarks, batch API, profiler in dev tools; fuzz JS/C++ boundary, soak tests, leak fixes | todo |
| 14 | `alpha init` templates, docs site + API reference + agent docs, 5+ examples, LICENSE/semver/CHANGELOG/CONTRIBUTING | todo |
| 15 | Android target (`alpha export --target android-arm64`, APK, touch) | todo |
| 16 | Shaders / post-processing / 2D lighting (SDL_GPU) | todo |
| 17a | Shipping bytecode + single file: `alpha export --standalone` ships `main.jsbc` (QuickJS bytecode via JS_WriteObject, compiled by the same-version release engine `alpha_star --compile`, debug info stripped) instead of main.js; engine accepts only bytecode stamped with its own version/build (clear error otherwise) and never from outside the pack; `--single-file` appends bytecode + assets as an archive to the shipping exe (engine reads itself, LOVE-fused style); dev/test/hot reload keep JS + source maps; check_shipping asserts no readable JS in shipped packs; measure boot time JS vs bytecode | todo |
| 17 | Release prep: npm per-platform engine packages, signing hooks, final verification | todo |

Out of reach here: macOS verification (no Mac), Xbox (needs GDK access), running CI on
GitHub and publishing (need the user's approval to push/publish).
