# Shaders and lighting

**What you'll build:** a dark scene lit by torches and a player lamp, with bloom and a vignette,
a CRT toggle, and a red "hit flash" that also bends the screen through a custom GLSL shader.

Post effects (`ctx.postfx`) and 2D lights are **render-only**: they change how a frame looks, never
the game. `simulate()`, `alpha test` and saved state cannot see them, so they cannot make a test
flaky. Game UI and dev tools are drawn on top, unaffected.

## Complete example

`src/main.ts`:

```ts
import { define } from "alpha";

interface Night {
  actions: "flash" | "crt";
  sounds: never;
  images: never;
  tags: "player" | "torch" | "crate";
  scenes: { play: undefined };
  ui: {};
  shaders: { wave: "amount" | "time" };      // custom shader -> its uniforms
}

const { scene, game } = define<Night>();

const play = scene("play", {
  state: () => ({ flash: 0 }),
  enter(ctx) {
    for (const x of [60, 160, 260]) {
      ctx.spawn({ tags: ["crate"], pos: [x, 130], shape: { rect: [24, 24], color: "#ab5236" } });
      ctx.spawn({ tags: ["torch"], pos: [x, 60], shape: { circle: 3, color: "#ffec27" },
        light: { radius: 70, color: "#ffb060", intensity: 1.2 } });
    }
    ctx.spawn({ tags: ["player"], pos: [40, 120], shape: { rect: [12, 16], color: "#29adff" },
      light: { radius: 50, color: "#a0c8ff" } });
    ctx.postfx.add("bloom", { strength: 0.8, radius: 3 });
    ctx.postfx.add("vignette", { strength: 0.6 });
    ctx.postfx.add("wave", { amount: 0 });
  },
  update(ctx) {
    ctx.find("player").pos.x = 40 + ((ctx.time * 60) % 240);
    if (ctx.input.pressed("crt")) {
      if (!ctx.postfx.remove("crt")) ctx.postfx.add("crt", { strength: 0.7 });
    }
    if (ctx.input.pressed("flash")) ctx.state.flash = 1;
    ctx.state.flash = Math.max(0, ctx.state.flash - ctx.dt * 3);
    ctx.postfx.set("wave", { amount: ctx.state.flash, time: ctx.time });
    if (ctx.state.flash > 0) ctx.postfx.add("tint", { color: "#ff4040", strength: ctx.state.flash });
    else ctx.postfx.remove("tint");
  },
});

export default game({
  config: {
    title: "Night", size: [320, 180], background: "#5f574f", seed: 1,
    lighting: { ambient: "#202840" },          // the color where no light reaches
    actions: { flash: ["Space"], crt: ["C"] },
  },
  start: "play",
  scenes: { play },
  assets: {
    sounds: {},
    shaders: { wave: { file: "assets/shaders/wave.frag", uniforms: { amount: 0.5, time: 0 } } },
  },
});
```

`assets/shaders/wave.frag`:

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

`test/main.test.ts`:

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

export function effectsNeverBreakTheGame() {
  const run = simulate(game, { seconds: 1, inputs: { flash: [0, 40], crt: [5, 20] } });
  run.expect.noErrors();
  run.expect.count("torch").equals(3);
}
```

Without Vulkan (and in headless tests) the `wave` shader is skipped with one logged warning; the
built-in effects and lighting still draw.

## Step by step

### 1. Built-in post effects

```ts
enter(ctx) {
  ctx.postfx.add("bloom", { strength: 0.9, radius: 3 });  // glow around bright pixels
  ctx.postfx.add("crt");                                   // scanlines + dark edges, defaults
},
```

| Effect | Params (default) |
|---|---|
| `bloom` | `strength` 0..2 (0.6), `radius` 1..16 game px (4) |
| `crt` | `strength` 0..1 (0.5) |
| `pixelate` | `size` 1..64 game px (4) |
| `tint` | `color` (`"#ffffff"`), `strength` 0..1 (1): the screen multiplied by the color |
| `vignette` | `strength` 0..1 (0.5), `radius` 0..1 (0.5): clear part from the center |

### 2. Change, remove, list

```ts
ctx.postfx.add("vignette", { strength: 0.5 });
ctx.postfx.add("vignette", { strength: 0.8 });   // already added: replaces params, keeps its place
ctx.postfx.set("vignette", { strength: 0.8 });   // same, but an error if it was not added
ctx.postfx.remove("crt");                        // false when it was not there
ctx.postfx.list();                               // names in draw order
ctx.postfx.clear();
```

Effects draw in the order they were **first** added. Params left out take their defaults (with
`set` too). Each scene starts with none.

### 3. 2D lights

```ts
config: { lighting: { ambient: "#404858" } },
// a scene: a light on any entity, or a lamp with no shape
ctx.spawn({ tags: ["lamp"], pos: [100, 80], light: { radius: 110, color: "#ffd9a0", intensity: 1.5 } });
```

With `config.lighting` the world is multiplied by a lightmap: `ambient` everywhere, plus each
light (full `color` x `intensity` at the entity, fading to nothing at `radius` world pixels).
Lights add up, follow their entity, its layer's parallax and the camera zoom. `radius` 1..4096,
`intensity` 0..8. Without `config.lighting`, lights are ignored. Lighting is applied before post
effects, so `bloom` makes lit areas glow.

### 4. A custom shader

```ts
// GameTypes: shaders: { gray: "amount" }
assets: { shaders: { gray: { file: "assets/shaders/gray.frag", uniforms: { amount: 1 } } } },
// a scene: same calls as the built-ins, uniforms by name
ctx.postfx.add("gray", { amount: 1 });
```

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

The contract (SDL_GPU fragment shader rules for SPIR-V):

- `#version 450`, entry point `main`.
- Inputs `layout(location = 0) in vec4` (vertex color) and `layout(location = 1) in vec2` (frame
  coordinates); output `layout(location = 0) out vec4`.
- The frame is a `sampler2D` at `layout(set = 2, binding = 0)`.
- Uniforms: one `std140` block at `layout(set = 3, binding = 0)` holding **only `float`s, in the
  order of `uniforms`**, at most 16 (no `vec2`/`vec4` members: use two or four floats). Leave the
  block out when there are no uniforms.

### 5. Animate a uniform

```ts
update(ctx) {
  ctx.postfx.set("wave", { amount: 0.8, time: ctx.time });
},
```

### 6. How shaders are built and shipped

- You write GLSL only. [glslang](https://github.com/KhronosGroup/glslang) compiles it to SPIR-V
  (Vulkan) and [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross) to MSL (Metal);
  both ship with the release engine.
- `alpha run` compiles the `.frag` at boot and on every save (hot reload). A compile error pauses
  the game naming `assets.shaders.<name>`, the file and the line
  (`shaders/wave.frag:3: 'nope' : undeclared identifier`).
- `alpha export` puts `wave.spv` and `wave.msl` in the pack instead of the `.frag`; shipping
  engines contain no compiler.

| Platform | Custom shaders |
|---|---|
| Windows, Linux | Vulkan (needs a Vulkan driver) |
| Android | Vulkan (Android 7+ devices with Vulkan) |
| macOS / iOS | not yet (packs already carry the `.msl`) |
| Direct3D 12 / Xbox | not yet |

Built-ins and lighting work on every renderer (Direct3D, Vulkan, Metal, OpenGL, GLES, software)
and appear in `--screenshot` and MCP captures. A game with `assets.shaders` asks for SDL's GPU
renderer on Vulkan at start; without Vulkan each custom effect is skipped with one warning.

## Recipes

### Hit flash on one entity

```ts
ctx.onHit("bullet", "enemy", (_bullet, enemy) => {
  enemy.tint = "#ff0000";
  ctx.after(0.08, () => { if (enemy.alive) enemy.tint = "#ffffff"; });
});
```

Or blink it: `ctx.tween(enemy, { to: { alpha: 0.2 }, seconds: 0.05, yoyo: true, repeat: 3 });`.

### Whole-screen damage flash

The `flash` value in the complete example: a red `tint` whose strength decays, removed at 0.

### Day / night cycle

`config.lighting.ambient` is fixed, so grade the screen with `tint`:

```ts
const hex = (v: number) => Math.round(v).toString(16).padStart(2, "0");
update(ctx) {
  const day = (Math.sin(ctx.time * 0.1) + 1) / 2;     // 0 = midnight, 1 = noon
  ctx.postfx.add("tint", {
    color: `#${hex(60 + 195 * day)}${hex(70 + 185 * day)}${hex(140 + 115 * day)}`,
    strength: 1,
  });
},
```

For lamps that only matter at night, keep `config.lighting` with a dark ambient in a night
scene and none in the day scene.

### Retro look

```ts
ctx.postfx.add("pixelate", { size: 3 });
ctx.postfx.add("crt", { strength: 0.6 });
```

### Toggle an effect from the keyboard

```ts
if (ctx.input.pressed("crt")) {
  if (!ctx.postfx.remove("crt")) ctx.postfx.add("crt");
}
```

### Respect reduced motion

```ts
if (!ctx.settings.reducedMotion) ctx.postfx.set("wave", { amount: 0.8, time: ctx.time });
```

## Gotchas

- Unknown effects or params and out-of-range values pause the game with an error listing what is
  allowed (`postfx "crt": unknown param "amount"; have: strength`). A shader may not reuse a
  built-in's name.
- Lights cannot be changed after spawn: destroy and respawn the entity (or a child lamp entity)
  to change one.
- `bloom` and `pixelate` draw the world at game size first: world text is then rasterized at game
  size, not window size.
- If a renderer cannot make render targets, lighting, `bloom` and `pixelate` are skipped with one
  logged notice; `crt`, `vignette` and `tint` still draw.
- Tests never see effects: assert game state, not looks.

## See also

- `examples/shaders` (`alpha init --template shaders`): six GLSL shaders with animated uniforms,
  lighting, touch buttons.
- [Text and languages](text.html) for how text interacts with bloom/pixelate.
- In `alpha.d.ts`: `PostFx`, `PostFxParams`, `ShaderAsset`, `LightSpec`, `GameConfig.lighting`.
