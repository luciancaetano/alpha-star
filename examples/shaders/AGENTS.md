# AGENTS.md — Shader Lab (alpha-star game)

A small lit scene (checkerboard floor, orbiting orbs with lights and spark trails) seen through
six custom full-screen shaders: wave, chroma (chromatic aberration), sepia, posterize, scanlines,
pulse (vignette). Left/right cycles them, up/down sets their intensity; phones get on-screen buttons.

You are editing a game for the alpha-star engine. Write TypeScript in `src/`; `alpha export`
type-checks it and bundles it into one JS module (`dist/main.js`) that the engine runs. The API
types live in `node_modules/@alpha-star/sdk/types/alpha.d.ts`: read that file when unsure, it
is the source of truth. The shader contract is in `node_modules/@alpha-star/sdk/llm/shaders.md`.

## Commands

```
npm install                 # once
npm run check               # type-check only (fast)
npm test                    # headless simulate() tests (test/main.test.ts)
npm start                   # export, then play
npx alpha run --watch       # every save (TS or .frag) re-exports and the game hot-reloads
npm run android:debug       # debug APK in android/ (the pack ships compiled .spv shaders)
```

A change is done only when `npm run export` prints `exported dist (...)` and `npm test` passes.

## Layout

| File | Holds |
|---|---|
| `src/types.ts` | Every name: actions, sounds, tags, shaders and their uniforms, UI, scenes. |
| `src/tuning.ts` | The shader cycle order (`SHADERS`), intensity step, orb orbit. |
| `src/cycle.ts` | Pure rules: cycling with wrap-around, intensity clamp, HUD labels. |
| `src/prefabs/scene.ts` | Floor tiles, orbs (shape + light + emitter), title text. |
| `src/scenes/lab.ts` | Input (`update`), `ctx.postfx` set/remove, touch buttons; `systems`: orb motion (`tag: "orb"`, `data.i`), the animated uniforms. |
| `assets/shaders/*.frag` | The six Vulkan GLSL fragment shaders. |

## How things work here

- **Custom shaders:** `assets.shaders.<name> = { file, uniforms }` (in `src/main.ts`); each `.frag`
  is `#version 450` with the frame at `set = 2, binding = 0` and a `std140` float block at
  `set = 3, binding = 0` whose members follow the `uniforms` order (`intensity`, `time`).
  `alpha run` compiles them with glslang; `alpha export` packs `.spv` instead.
- **One at a time:** `ctx.postfx.remove(old)` then `ctx.postfx.set(new, uniforms)`; every frame
  `ctx.postfx.set(name, { intensity, time: ctx.time })` animates it. A wrong uniform name is an
  error, so the tests' full lap catches asset/uniform mismatches.
- **Render-only:** shaders need Vulkan (Windows, Linux, Android); elsewhere and in headless tests
  they are skipped with a warning. Never put game logic in a shader.
- **New shader:** add the `.frag`, its name to `types.ts` (`shaders`), `main.ts` (`assets.shaders`)
  and `SHADERS` in `tuning.ts`; update the count in the tests.
- **Touch:** `ctx.platform.mobile` shows the `touch` UI; its buttons call the same functions as keys.
- **Determinism:** `ctx.time` drives motion; the same inputs always give the same run.

F12 opens the dev tools while the game runs (not in exported standalone games).
