# Custom Shaders

> summary: Custom full-screen Vulkan GLSL fragment shaders (assets.shaders + ctx.postfx): the GLSL contract (sets, locations, float std140 uniforms), wave and grayscale examples, how alpha run compiles (glslang to SPIR-V, shadercross to MSL) and alpha export packs .spv + .msl, and the Vulkan requirement per platform; read when a built-in post effect is not enough.

```ts
// types.ts: GameTypes gets  shaders: { wave: "amount" | "time"; gray: "amount" }   (never: no uniforms)
assets: { shaders: {
  wave: { file: "shaders/wave.frag", uniforms: { amount: 0.5, time: 0 } },
  gray: { file: "shaders/gray.frag", uniforms: { amount: 1 } },
} },
ctx.postfx.set("wave", { amount: 0.8 });                 // like a built-in effect
ctx.postfx.set("wave", { amount: 0.8, time: ctx.time }); // every frame for animation (missing = default)
ctx.postfx.remove("wave");
```

`shaders/wave.frag`:

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

`shaders/gray.frag`:

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

## Rules

- Contract: `#version 450`, entry `main`; in `location 0` vec4 color, `location 1` vec2 uv; out `location 0` vec4; frame `sampler2D` at `set = 2, binding = 0`; uniforms one `std140` block at `set = 3, binding = 0`.
- Uniforms: only `float` members, in `uniforms` order, at most 16 (no vec2/vec4: split into floats). Omit the block when there are no uniforms.
- Only `file` (.frag) and `uniforms` (`{ name: default number }`). A shader name may not be a built-in effect name (bloom crt pixelate tint vignette). At most 32 shaders.
- `alpha run` compiles with glslang on boot and on every save (hot reload); compile errors pause the game: `assets.shaders.wave: shaders/wave.frag did not compile:` then one line per error: `ERROR: shaders/wave.frag:3: 'nope' : undeclared identifier`. `alpha export` writes `<name>.spv` (Vulkan) and `<name>.msl` (Metal, made from the SPIR-V by shadercross) next to where the .frag was and leaves the .frag out. Write GLSL only: never HLSL or MSL by hand.
- Runs on Vulkan only: Windows, Linux, Android. macOS/iOS: not yet (packs already carry the .msl; no macOS/iOS engine yet). Without Vulkan (or the software renderer in headless tests) each effect is skipped with one logged warning: never rely on a shader for game logic. Render-only: `simulate()` never sees it.
- Errors: `postfx "wave": unknown uniform "speed"; have: amount time`; `assets.shaders.wave: no compiled shader for "shaders/wave.frag" (looked for shaders/wave.spv ...)`.

See also examples/shaders (`alpha init --template shaders`): six GLSL shaders (wave, chroma, sepia, posterize, scanlines, pulse) cycled with ctx.postfx, intensity + time uniforms, tests that catch bad assets and uniform names.
