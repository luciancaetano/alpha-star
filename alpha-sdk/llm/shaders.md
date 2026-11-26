# Custom Shaders

> summary: Custom full-screen HLSL fragment shaders (assets.shaders + ctx.postfx): the shader contract, uniforms, how alpha run compiles and alpha export packs them, and the GPU-renderer requirement; read when a built-in post effect is not enough.

```ts
// types.ts: GameTypes gets  shaders: { wave: "amount" | "time" }   (never: no uniforms)
assets: { shaders: { wave: { file: "shaders/wave.hlsl", uniforms: { amount: 0.5, time: 0 } } } },
ctx.postfx.add("wave", { amount: 0.8 });                 // like a built-in effect
ctx.postfx.set("wave", { amount: 0.8, time: ctx.time }); // every frame for animation (missing = default)
ctx.postfx.remove("wave");
```

```hlsl
cbuffer Params : register(b0, space3) { float amount; float time; };  // floats, in `uniforms` order
Texture2D u_texture : register(t0, space2);                           // the frame
SamplerState u_sampler : register(s0, space2);
struct PSInput { float4 v_color : COLOR0; float2 v_uv : TEXCOORD0; };
float4 main(PSInput input) : SV_Target {                              // entry point: main
    return u_texture.Sample(u_sampler, input.v_uv) * input.v_color;
}
```

## Rules

- Only `file` (.hlsl) and `uniforms` (`{ name: default number }`, at most 16, cbuffer order). Omit the cbuffer when there are no uniforms.
- A shader name may not be a built-in effect name (bloom crt pixelate tint vignette). At most 32 shaders.
- `alpha run` compiles on boot and on every save (hot reload); compile errors pause the game with the compiler output. `alpha export` writes `<name>.spv/.dxil/.msl` next to where the .hlsl was and leaves the .hlsl out.
- Needs SDL's GPU renderer (Vulkan/D3D12/Metal). Otherwise (software renderer in headless tests, no GPU) the effect is skipped with one logged warning: never rely on a shader for game logic. Render-only: `simulate()` never sees it.
- Errors: `postfx "wave": unknown uniform "speed"; have: amount time`; `assets.shaders.wave: no compiled shader for "shaders/wave.hlsl" (looked for ...)`.
