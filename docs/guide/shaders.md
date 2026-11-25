# Shaders and lighting

Post effects and 2D lights are **render-only**: they change how a frame looks, never the game.
`simulate()`, `alpha test` and saved state cannot see them, so turning one on cannot make a test flaky.
They work on every renderer the engine uses (Direct3D, Vulkan, Metal, OpenGL, GLES and the software
renderer that headless tests and `--hidden` screenshots use), and MCP / `--screenshot` captures include
them. Game UI (RmlUi) and dev tools are drawn on top, unaffected.

## Post effects: `ctx.postfx`

```ts
enter(ctx) {
  ctx.postfx.add("bloom", { strength: 0.9, radius: 3 });  // glow around bright pixels
  ctx.postfx.add("crt");                                    // scanlines + dark edges
},
update(ctx) {
  if (ctx.input.pressed("crt")) ctx.postfx.list().includes("crt") ? ctx.postfx.remove("crt") : ctx.postfx.add("crt");
},
```

| Effect | Params (default) |
|---|---|
| `bloom` | `strength` 0..2 (0.6), `radius` 1..16 game px (4) |
| `crt` | `strength` 0..1 (0.5) |
| `pixelate` | `size` 1..64 game px (4) |
| `tint` | `color` (`"#ffffff"`), `strength` 0..1 (1): the screen multiplied by the color |
| `vignette` | `strength` 0..1 (0.5), `radius` 0..1 (0.5): clear part from the center |

- Effects run in the order they were **first** added; `add` on an effect already there replaces its
  params in place. `set(name, params)` does the same but fails when the effect was not added.
  `remove(name)` returns false when it was not there; `clear()`; `list()` gives names in draw order.
- Each scene starts with no effects (they belong to the scene, like the camera).
- Unknown effects or params and out-of-range values pause the game with an error that lists what
  is allowed, e.g. `postfx "crt": unknown param "amount"; have: strength`.
- `bloom` and `pixelate` draw the world into an offscreen texture at the game's size first; world
  text is then rasterized at game size rather than window size while they are on.

## 2D lighting

```ts
config: { lighting: { ambient: "#404858" } },   // the color where no light reaches
// ...
ctx.spawn({ tags: ["torch"], pos: [100, 80], light: { radius: 110, color: "#ffd9a0", intensity: 1.5 } });
```

With `config.lighting`, the world is multiplied by a lightmap: `ambient` everywhere, plus each
entity's `light` (full `color` x `intensity` at the entity, fading to nothing at `radius` world
pixels). Lights add up, follow their entity (interpolated like shapes), its layer's parallax and the
camera zoom. Without `config.lighting`, lights are ignored. A light needs no shape: spawn
`{ tags, pos, light }` for a lamp. Limits: `radius` 1..4096, `intensity` 0..8.

Lighting is applied before post effects, so `bloom` makes lit areas glow.

## How it works, and custom shaders

The engine renders with SDL 3.2's `SDL_Renderer` on every platform. Effects are built from what
every SDL renderer supports: offscreen render targets, blend modes (add, modulate) and vertex
colors, so they behave the same on a GPU and on the software renderer. If a renderer cannot make
render targets, lighting, `bloom` and `pixelate` are skipped with one logged notice; the overlays
(`crt`, `vignette`, `tint`) still draw.

Custom shader code (`assets.shaders`) is not available yet: SDL 3.2 has no way to run a custom
shader inside `SDL_Renderer`. It is planned with the move to SDL 3.4 (`SDL_GPURenderState`), with
one source language cross-compiled at export.
