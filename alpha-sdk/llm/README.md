# alpha-star LLM Docs

> summary: Index of concise docs that teach an LLM coding agent the alpha-star game script API; read this to pick the file for the task at hand.

Source of truth: `types/alpha.d.ts` (modules `"alpha"`, `"alpha/test"`). Real usage: `examples/*/src` and `templates/`. Start with overview.md.

| File | Summary |
|---|---|
| [overview.md](overview.md) | Project layout, the types.ts/alpha.ts/scenes/main.ts pattern, GameConfig, the scene lifecycle and ctx, and the alpha CLI; read this first before writing any alpha-star game code. |
| [entities.md](entities.md) | EntitySpec fields, spawn/find/destroy, prefabs as plain functions, tags, parent/child, entity data, looks and bulk positions; read when creating or changing game objects. |
| [sprites-and-images.md](sprites-and-images.md) | Image assets, sprite sheets and animations, flips, texture filtering (filter nearest\|linear\|sharp, config.pixelMode) and atlas packing; read when drawing images or animated sprites. |
| [input.md](input.md) | Actions and axes (config.actions/axes), device input names, mouse/touch, ctx.platform hints, multiple players, rumble and rebinding; read when wiring controls or reading player input. |
| [physics.md](physics.md) | Box2D-backed bodies, colliders, collision categories, contact handlers, raycasts/area queries, forces/impulses and joints; read when anything should move physically, collide or be detected. |
| [tilemaps.md](tilemaps.md) | Loading Tiled (.tmj) maps, spawnMap with collider layers and draw layers, reading object layers as spawn points; read when building levels from Tiled. |
| [camera-and-window.md](camera-and-window.md) | ctx.camera (follow, bounds, zoom, shake, toWorld), draw layers with parallax, config.window scaling/flags and ctx.window fullscreen; read when the view must move or the window must look a certain way. |
| [text-and-fonts.md](text-and-fonts.md) | World text shapes, font assets (TTF/OTF, BMFont .fnt, built-in 8x8), pixelMode for fonts and font licenses; read when showing text in the world or choosing fonts for world text and UI. |
| [ui.md](ui.md) | Game UI with RmlUi .rml/.rcss documents: declaring docs and element ids, show/hide/set/on, focus and gamepad navigation, accessibility hooks and UI tests; read when building HUDs, menus or buttons. |
| [audio.md](audio.md) | Sound assets (files and synthesized notes), playback options, streamed music with fades/crossfades, and volume buses; read when adding sound effects or music. |
| [particles-and-effects.md](particles-and-effects.md) | Particle emitters on spawn specs, looks (tint/alpha/blend), 2D lighting (config.lighting + entity light) and full-screen post effects (ctx.postfx); read when adding visual effects. |
| [shaders.md](shaders.md) | Custom full-screen Vulkan GLSL fragment shaders (assets.shaders + ctx.postfx): the GLSL contract (sets, locations, float std140 uniforms), wave and grayscale examples, how alpha run compiles (glslang) and alpha export packs .spv, and the Vulkan requirement per platform; read when a built-in post effect is not enough. |
| [tweens-and-timers.md](tweens-and-timers.md) | ctx.tween on entities and plain objects (eases, yoyo, repeat, onDone), ctx.every/ctx.after timers and the vec helpers; read when animating values over time or scheduling work. |
| [saves-and-i18n.md](saves-and-i18n.md) | Persistent save slots with versioning/migration, player settings (volumes, locale, rebinds, accessibility flags) and localization with ctx.t, plurals and data-t; read when persisting progress, adding options menus or translating text. |
| [testing.md](testing.md) | Headless deterministic tests with alpha/test simulate(), its options (inputs, clicks, saves, settings, appPauses, platform) and checks (expect, entities, state); read before writing or debugging test/main.test.ts. |
| [exporting.md](exporting.md) | alpha export packs, standalone builds with the shipping engine and game.jsbc bytecode, signing, other targets and Android APKs; read when building something a player will run. |
| [mcp.md](mcp.md) | The alpha mcp server's tools for agents (check, test, export, start, step, input, state, screenshot, logs, reload) and the play-test loop; read when driving a running game as an AI agent. |
| [troubleshooting.md](troubleshooting.md) | Common errors an agent hits (type errors, boot/asset errors, runtime script errors, engine/SDK version mismatches, MCP and export failures) and what each means; read when a check, test, run or export fails. |
