# Camera and Window

> summary: ctx.camera (follow, bounds, zoom, shake, toWorld), draw layers with parallax, config.window aspect/scale (Godot-style: fill the screen, ctx.camera.view = visible world) and flags, ctx.window fullscreen, ctx.safeArea and UI class="safe" (phones: immersive fullscreen, cutouts); read when the view must move or the window must look a certain way.

## Camera API

```ts
ctx.camera.pos                  // Vec2Ref: world point at the screen center
ctx.camera.zoom = 2;            // > 0
ctx.camera.rotation = 0;        // degrees; the world turns the other way
ctx.camera.follow(entity | null, { lerp?: (0, 1] /*1 = locked*/, deadzone?: [w, h] });
ctx.camera.shake(intensity, seconds);   // deterministic, fades out
ctx.camera.bounds([x, y, w, h] | null); // keep the visible area inside
ctx.camera.toWorld(ctx.input.mouse()): Vec2
```

Each scene starts with the identity camera (pos = center of `config.size`, zoom 1). The camera moves on the fixed step; UI never moves with it.

## Layers

```ts
// types.ts: layers: "far" | "fx";
config: { layers: { far: { z: -100, parallax: 0.2 }, fx: { z: 100 } } }
ctx.spawn({ ...hill(x), layer: "far" });
```
Drawn in layer `z` order, then entity `z`. `parallax` 0 = fixed to screen (HUD-like world items), 1 = world (default). Unnamed default layer: z 0, parallax 1.

## Window

```ts
config: {
  size: [320, 180],             // game pixels
  window: {
    aspect: "expand",           // expand (default) | keep_width | keep_height | keep
    scale: "fractional",        // fractional (default) | integer
    fullscreen: false, resizable: true, altEnter: true,
    borderless: false, alwaysOnTop: false, maximized: false, vsync: true,
  },
}
ctx.window.fullscreen = true;   // switches at end of frame; persisted in settings
ctx.window.size                 // real window pixels (read-only)
ctx.camera.view                 // [x, y, w, h] world rect the screen shows now (read-only)
ctx.safeArea                    // [x, y, w, h] in screen pixels no cutout/rounded corner hides (read-only)
```

```html
<body class="safe">  <!-- the engine pads every class="safe" element by the safe area -->
```

- `config.size` is the design canvas (world and UI units), not the window: a bigger size makes everything smaller on screen. The first window is 2x it (integer scale) or 1.5x, shrunk to fit 90% of the screen (integer never below 1x).
- Design at `config.size`; the engine scales it to the screen keeping the aspect (Godot stretch aspect). `aspect`: `expand` (default: more world on the longer axis, never bars), `keep_width` (width fixed, taller screens show more height, wider get side bars: portrait games like flappy), `keep_height` (height fixed, wider screens show more width: side scrollers), `keep` (exactly config.size, letterbox bars). `scale: "integer"`: whole multiples only (crisp pixel art); the revealed axes still fill the screen up to under one scale step.
- The revealed world splits evenly around the design center (the default camera stays at config.size / 2). Size backgrounds, ground and walls to `ctx.camera.view` and spawn at its edges; keep gameplay that must not change (gap ranges, jump heights) in design numbers. `simulate(game, { size: [1080, 2340] })` runs a test on a tall phone.
- Screen pixels span the whole revealed screen: `[0, 0]` is the real top-left. UI documents (RmlUi) fill it too and scale with the kept axis (like Unity's Canvas Scaler matching width or height); put `class="safe"` on a HUD root (with `box-sizing: border-box` if it is `width/height: 100%`) so it stays inside cutouts.
- Phones always run fullscreen (immersive: no status or navigation bar; the game draws into the display cutout). Keep HUD and touch buttons inside `ctx.safeArea` (the whole screen on desktop). Shader and postfx passes cover the whole revealed screen.
- `config.window.scaling` was removed: `letterbox` -> `aspect: "keep"`, `integer` -> `scale: "integer"`, `expand` -> the default; `stretch`/`crop` are gone (use `expand` + `ctx.camera.view`). The error says the replacement.
- `vsync: false` draws as fast as possible (more CPU/GPU).
- Headless runs ignore window changes except reading back. Window size and title are fixed at launch (not hot-reloaded).

## Example (examples/platformer)

```ts
ctx.camera.bounds([0, 0, map.size[0], map.size[1]]);
ctx.camera.follow(hero, { lerp: CAMERA.lerp, deadzone: CAMERA.deadzone });
if (ctx.state.fallSpeed > PLAYER.hardLanding) ctx.camera.shake(CAMERA.shake, CAMERA.shakeSeconds);
```

## Rules and gotchas

- `camera.bounds` must be `[x, y, width, height]` or null, else an error.
- `ctx.settings.reducedMotion` scales shake intensity to 1/4.
- Mouse is in screen (game) pixels: convert with `toWorld` before comparing to entity positions.
- World text is rasterized at screen resolution x zoom, so it stays sharp when zoomed.
