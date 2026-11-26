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

Write your own full-screen effect as a Vulkan GLSL fragment shader (`.frag`), declare it, and use
it through `ctx.postfx` like a built-in:

```ts
// types.ts: shaders: { wave: "amount" | "time"; gray: "amount" }
assets: {
  shaders: {
    wave: { file: "shaders/wave.frag", uniforms: { amount: 0.5, time: 0 } },
    gray: { file: "shaders/gray.frag", uniforms: { amount: 1 } },
  },
},
// a scene
enter(ctx) { ctx.postfx.add("wave", { amount: 0.8 }); },
update(ctx) { ctx.postfx.set("wave", { amount: 0.8, time: ctx.time }); },
```

The shader contract (SDL_GPU's fragment shader rules for SPIR-V):

- `#version 450`; the entry point is `main`.
- Inputs: `layout(location = 0) in vec4` (vertex color) and `layout(location = 1) in vec2` (frame
  coordinates); output: `layout(location = 0) out vec4`.
- The frame is a `sampler2D` at `layout(set = 2, binding = 0)`.
- Uniforms are one `std140` block at `layout(set = 3, binding = 0)` holding **only `float`s, in the
  order of `uniforms`** (at most 16; no `vec2`/`vec4` members: use two or four floats). Leave the
  block out when the shader has no uniforms.

A wave that bends the frame over time:

```glsl
#version 450
layout(location = 0) in vec4 v_color;                    // vertex color
layout(location = 1) in vec2 v_uv;                       // frame coordinates
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_frame;  // the frame
layout(set = 3, binding = 0, std140) uniform Params {    // floats, in `uniforms` order
    float amount;
    float time;
};

void main() {
    vec2 uv = v_uv + vec2(sin(v_uv.y * 40.0 + time * 6.0) * 0.004 * amount, 0.0);
    o_color = texture(u_frame, uv) * v_color;
}
```

Grayscale, mixed by `amount`:

```glsl
#version 450
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
layout(set = 2, binding = 0) uniform sampler2D u_frame;
layout(set = 3, binding = 0, std140) uniform Params { float amount; };

void main() {
    vec4 c = texture(u_frame, v_uv) * v_color;
    float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    o_color = vec4(mix(c.rgb, vec3(g), amount), c.a);
}
```

- **Compiled by glslang.** [glslang](https://github.com/KhronosGroup/glslang) (built with the
  engine, staged next to the release engine) compiles the GLSL to SPIR-V.
- **`alpha run`** compiles the `.frag` when the game boots and again on every save (hot reload);
  a compile error pauses the game naming `assets.shaders.<name>`, the file and the line
  (`shaders/wave.frag:3: 'nope' : undeclared identifier`).
- **`alpha export`** puts the compiled `shaders/wave.spv` in the pack instead of the `.frag`;
  shipping engines only load `.spv` and contain no compiler.
- Custom shaders run on **Vulkan**: a game with `assets.shaders` asks for SDL's GPU renderer on the
  Vulkan driver at start. Without Vulkan (or on the software renderer of headless tests) the game
  uses SDL's default renderer and each custom effect is **skipped** with one logged warning; the
  game keeps running and the built-in effects still work.

| Platform | Custom shaders |
|---|---|
| Windows | Vulkan (needs a Vulkan driver; most GPUs since ~2016 have one) |
| Linux | Vulkan |
| Android | Vulkan (Android 7+ devices with Vulkan) |
| macOS / iOS | not yet: planned through Metal (SPIR-V -> MSL at export); the effect is skipped |

- Effect names share one namespace: a shader may not be called like a built-in (`crt`, ...).
  Unknown uniforms, non-numbers and more than 16 uniforms are boot or script errors that list what
  the shader has.

A complete example: `examples/shaders` (`alpha init --template shaders`) cycles six GLSL shaders with
animated uniforms and tests every asset and uniform name.
