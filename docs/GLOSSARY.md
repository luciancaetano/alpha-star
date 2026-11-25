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
| Frame | Everything drawn in one frame (`render::Frame`): background, ordered shapes, texts. |
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
