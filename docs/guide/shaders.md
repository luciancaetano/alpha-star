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

## How the built-in effects work

The engine renders with SDL 3.4's `SDL_Renderer` on every platform. The built-in effects use only
what every SDL renderer supports: offscreen render targets, blend modes (add, modulate) and vertex
colors, so they behave the same on a GPU and on the software renderer. If a renderer cannot make
render targets, lighting, `bloom` and `pixelate` are skipped with one logged notice; the overlays
(`crt`, `vignette`, `tint`) still draw.

## Custom shaders: `assets.shaders`

Write your own full-screen effect as an HLSL fragment shader, declare it, and use it through
`ctx.postfx` like a built-in:

```ts
// types.ts: shaders: { wave: "amount" | "time" }
assets: {
  shaders: { wave: { file: "shaders/wave.hlsl", uniforms: { amount: 0.5, time: 0 } } },
},
// a scene
enter(ctx) { ctx.postfx.add("wave", { amount: 0.8 }); },
update(ctx) { ctx.postfx.set("wave", { amount: 0.8, time: ctx.time }); },
```

The shader contract (SDL's GPU renderer conventions): the frame is `t0, space2` with its sampler at
`s0, space2`; the uniforms are one `cbuffer` at `b0, space3` holding the declared floats **in the
order of `uniforms`** (at most 16); the entry point is `main`.

```hlsl
cbuffer Params : register(b0, space3) { float amount; float time; };
Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);
struct PSInput { float4 v_color : COLOR0; float2 v_uv : TEXCOORD0; };

float4 main(PSInput input) : SV_Target {
    float2 uv = input.v_uv + float2(sin(input.v_uv.y * 40.0 + time * 6.0) * 0.004 * amount, 0);
    return u_texture.Sample(u_sampler, uv) * input.v_color;
}
```

- **One source, every GPU.** [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross)
  compiles the HLSL to SPIR-V (Vulkan, Android), DXIL (Direct3D 12) and MSL (Metal).
- **`alpha run`** compiles the `.hlsl` when the game boots and again on every save (hot reload);
  a compile error pauses the game with the compiler's message, naming `assets.shaders.<name>`.
- **`alpha export`** puts the compiled blobs (`shaders/wave.spv`, `.dxil`, `.msl`) in the pack
  instead of the `.hlsl`; shipping engines only load blobs and contain no compiler.
- Custom shaders need **SDL's GPU renderer**, which a game with `assets.shaders` asks for at start.
  If it is not available (no Vulkan / D3D12 / Metal device), or the renderer is another one (the
  software renderer of headless tests), the effect is **skipped** with one logged warning; the game
  keeps running and the built-in effects still work.
- Effect names share one namespace: a shader may not be called like a built-in (`crt`, ...).
  Unknown uniforms, non-numbers and more than 16 uniforms are boot or script errors that list what
  the shader has.
