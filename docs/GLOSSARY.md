# Glossary

| Term | Meaning |
|---|---|
| Agent | An AI model that writes game scripts through the script API and MCP. |
| Vibecoder | End user who describes a game in natural language; never writes code. |
| Core | Dependency-free C++ domain code in `src/core`; pure, fast to test. |
| Entity | ECS identifier (EnTT) for a game object; scripts see an opaque handle. |
| Script API | The QuickJS-ng surface agents program against. The product. |
| Gate | `tools/gate.py`: build + tests + limits + CRAP + duplication. |
| World | One scene's entities (`ecs::World`): spawn, deferred destroy, find by tag in creation order, transform/velocity, `step(dt)` → hit/enter events, draw list. |
| EntityId | Opaque `uint32` handle (EnTT id + version); a stale handle is detected as dead, never reused silently. |
| Tag | Interned name (`TagRegistry`, max 64); entities carry a `TagSet` bitset. |
| Action | Named input (`Actions`) bound to device input names (`input_names()`); has pressed/down/released per fixed step. |
| Fixed step | Simulation always advances 1/60 s (`FixedStep`); real time only decides how many steps run. |
| Hit / Enter | Physics events: solid contact began / overlap with a sensor began (`a` = visitor, `b` = sensor). |
| Frame | Everything drawn in one frame (`render::Frame`): background, ordered shapes, and the engine's own error overlay lines (debug font; never a game API). |
| Host | `script::Host`: loads one compiled game module, boots it, steps it, builds frames. Any JS error pauses the game and is shown on screen. |
| Runtime | `src/script/runtime.js`, embedded in the binary: scenes, `ctx`, entity proxies, timers, collision dispatch. Talks to C++ only via `__native`. |
| Native | C++ function exposed to the runtime on `__native`; validates every argument and throws errors that name the field. |
| Asset | File or synth spec declared in `game.assets`; every asset is loaded at boot (preload) before the start scene. Paths are relative to the game `.js`. |
| Synth | Procedural sound spec: a note `{ wave, freq, duration, volume?, attack?, release? }` or a list of notes. Same spec → same samples (`core::synthesize`). |
| Mixer | `core::Mixer`: voices with volume/pitch/loop over shared sample buffers; main thread only, drained into the device each frame (`platform::AudioOut`). |
| Voice | One playing sound, returned by `ctx.audio.play` / `tone`; `stop()`, `playing`. |
| Sprite | Shape `{ image, size? }`: an image asset drawn as a rotated texture in z order. |
| Call budget | `kCallBudget` (250 ms, `script/js_engine.hpp`): wall time one script call (import, enter, update, handler) may run; past it QuickJS is interrupted and the game pauses with "script ran longer than 250 ms ... (infinite loop?)". |
| Memory limit | `kMemoryLimitMb` (256 MB): QuickJS heap cap; hitting it pauses with "script ran out of memory (limit 256 MB)". |
| Save path | `SavePathFor(title)`: where `ctx.save` persists; the engine uses `SDL_GetPrefPath("alpha-star", title)/save.json` (per-user data dir), tests pass a temp file. |
| Game instance | One running game inside the JS context: runtime state in a `createInstance(id, save)` closure (`runtime.js`) + native `ScriptState` under the same id in `Instances` (`script/instances.hpp`). The engine's game is instance 0; each `simulate()` is a fresh one, destroyed when the run ends. Natives act on the *active* instance; the runtime selects it around every call. |
| simulate | `alpha/test`: runs a game headlessly in a fresh instance for `round(seconds*60)` fixed 1/60 steps, pressing actions (per player: `inputs` may be an array), holding axis values (`axes: { name: [[frame, value]] }`, held until the next entry) and clicking UI elements (`clicks: { "doc#id": [frames] }`) on given 0-based frames; in-memory save, no window, no audio. Returns `Run { frames, expect }` from a snapshot (scene, tag counts, UI texts/visibility, error). |
| Test mode | `alpha_star --test <tests.js>`: imports a bundled test module, calls each exported function once (name order), prints `PASS`/`FAIL` lines, exit 0 iff all pass. `alpha test` bundles `test/main.test.ts` to `build/test/main.test.js` and runs it. |
| Source map | esbuild's inline v3 map at the end of a bundle; `core::SourceMap` (pure VLQ decoder) + `script::load_bundle_map`; `Host` rewrites `<bundle>:LINE:COL` in errors to `src/...ts:LINE:COL` (overlay, stderr, test failures). Sources are relative to the bundle's final location. |
| Dev tools | Built-in engine tools on Dear ImGui (`src/devtools/`), toggled with F1 or gamepad Back+Start, drawn over the game frame before present: overlay, entity inspector, script console, time controls, physics view. Only in `ALPHA_DEVTOOLS` builds (dev preset); release/shipped engines contain none of it. Never visible to scripts. Game UI is a separate system. |
| DevSnapshot | `devtools::snapshot(host, FrameStats)`: plain data the overlay shows (fps, frame ms, fixed steps this frame, last step's script call ms vs the 250 ms budget, entities, mixer voices, scene, error). |
| Log ring | `core::LogRing`: newest N (200) `console.log/warn/error` lines and pausing script errors, shared by every game instance (`Instances::log`); the dev tools console reads it. console.* also prints `[log|warn|error] text` to stderr. |
| Control channel | `alpha-control` protocol (`src/mcp/`, `ALPHA_MCP`: dev + release, never shipping): `alpha_star <pack> --control PORT [--hidden]` listens on 127.0.0.1 only, one client, non-blocking sockets polled on the main thread after each frame is drawn (before present). Line-delimited flat JSON: `{"id", "cmd", ...}` -> `{"id", "ok", ...}` or `{"ok": false, "error"}`; greeting first; with `$ALPHA_CONTROL_TOKEN` set, `hello {token}` must come first. Commands: hello state pause resume step press hold release axis click screenshot logs quit. |
| Agent input | `mcp::AgentInput`: actions the agent pressed (one step) or holds, OR-ed into each step's device `InputFrame` through the action's bound inputs, so the game sees a device; axes via `core::Axes::hold` (held across updates until released). |
| Agent pause | The control channel's pause: the clock runs no steps; `step N` runs N fixed steps synchronously; independent of the dev tools' and the script-error pause. |
| alpha mcp | The SDK's MCP server (stdio, `@modelcontextprotocol/sdk`): tools `alpha_check/export/test` and `game_start/state/step/pause/resume/input/click/screenshot/logs/stop` over the control channel of a release engine it launches with a random token. |
| Time control | `devtools::TimeControl`: dev-tools pause over the fixed-step clock; paused runs 0 steps except exactly one after each Step request. Independent of a script-error pause. |
| Physics view | Dev tools toggle: each body's collider (`World::colliders`, from the `Collider` component) drawn as an unfilled outline over the game (green solid, yellow sensor). |
| Game UI | RmlUi (`src/ui/`): HTML/CSS-like `.rml` documents + `.rcss` styles, declared in `assets.ui`, fonts in `assets.fonts` (name = `font-family`). One `ui::Ui` (Rml::Context, game logical size) per game instance; documents load hidden at boot, reload hidden on every scene change. Script API `ctx.ui.show/hide/set/on`. No default stylesheet: `div`/`p` are inline unless the RCSS says `display: block`. |
| UI handler | Function registered with `ctx.ui.on(doc, id, "click" \| "back", fn)`; events queue the handler id and the runtime runs queued handlers at the start of the next step (deterministic, never re-entrant). |
| Input consumption | `ui::consumes`: an input the game UI acted on (click on an element with a handler or a form control, focus move, Enter/Space/A on a focused element, Escape/B with a "back" handler, typing in a text field) never also reaches game actions. Order: dev tools, then game UI, then game input. |
| PadNav | `ui::PadNav`: gamepad focus navigation from the D-pad and left stick (deadzone 0.5, first move at once, repeat after 400 ms then every 120 ms). |
| Build flavor | A set of `ALPHA_*` compile flags (`cmake/Flavors.cmake`), always defined 0/1 and tested with `#if`. Presets: **dev** (everything ON, tests), **release** (`ALPHA_DEBUG_TOOLS` + `ALPHA_MCP`: what `alpha run`/`alpha test`/`alpha mcp` use), **shipping** (`ALPHA_SHIPPING`: what players get). |
| Shipping engine | `ALPHA_SHIPPING` build: no dev tools, MCP, debug CLI flags, test runner or `alpha/test` (importing it lists the built-in modules that exist); IPO/LTO, stripped, no console window; every CLI flag ignored; logs (console.*, script errors, SDL_Log) go to `<pref dir>/log.txt`. Staged at `alpha-sdk/engine/<target>/shipping/`, copied by `alpha export --standalone`, verified by `tools/check_shipping.py`. |
| Pref dir | Per-user data dir `SDL_GetPrefPath("alpha-star", <config.title>)` (`platform::pref_file`): `save.json` and, in shipping, `log.txt`. |
| Player | 1..4 (`core::kMaxPlayers`; 0-based inside the engine). Player 1 owns keyboard, mouse and the first gamepad slot. `ctx.input.pressed/down/released/axis` = any player; `ctx.input.player(n)` = one; `ctx.input.players()` = 1 up to the highest connected gamepad's player. |
| Pad slot | `core::PadSlots`: player slot per gamepad in connection order. A disconnected slot stays reserved for that device kind (GUID) and goes back to it on reconnect; new devices take a never-used slot first, then any free one; a fifth pad is ignored. `platform::Gamepads` opens/closes SDL gamepads into slots. |
| Analog | Raw gamepad axis (`core::Analog`): `GamepadLeftX/LeftY/RightX/RightY` in [-1, 1] (+y down), `GamepadLT/RT` in [0, 1]. Its digital view (`analog_buttons`, past 0.5) is bindable in actions: `GamepadLeftX-`/`+`, `GamepadLeftY-` (= up), ..., `GamepadLT`, `GamepadRT`. |
| Axis | Named analog control in `config.axes` (`core::Axes`): sources are analog names or `"Neg/Pos"` input pairs (`"A/D"`, reading -1/0/1); the axis reads the strongest source, clamped to [-1, 1]. |
| Deadzone | `config.deadzone` (default 0.2, 0 <= d < 1): radial on each stick (vector length rescaled, direction kept), axial on triggers; values past it rescale to reach 1. Applies to axes, not to the digital view (threshold 0.5) nor to the UI's PadNav (0.5). |
| Input frame | `core::InputFrame`: what every player held (`InputSet`) plus raw analog values during one fixed step, and the connected player count; produced by `platform::consume`, fed to `core::Controls` (actions + axes + players + rumble requests). |
| Rumble | `ctx.input.rumble(strength 0..1, seconds 0..10, player?)` (all players if omitted): validated, queued per player (latest wins) in `core::Controls`, taken by the main loop each frame and started with `SDL_RumbleGamepad`; no-op without a pad and in simulate. |
| Unmapped joystick | A joystick SDL's built-in gamepad database (plus `SDL_GAMECONTROLLERCONFIG`) cannot map: reported once per GUID in the log (debug-tools builds), never a player. |
