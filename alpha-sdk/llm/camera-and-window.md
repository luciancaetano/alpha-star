# Camera and Window

> summary: ctx.camera (follow, bounds, zoom, shake, toWorld), draw layers with parallax, config.window scaling/flags and ctx.window fullscreen; read when the view must move or the window must look a certain way.

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
    scaling: "integer",         // letterbox (default) | integer | stretch | expand | crop
    fullscreen: false, resizable: true, altEnter: true,
    borderless: false, alwaysOnTop: false, maximized: false, vsync: true,
  },
}
ctx.window.fullscreen = true;   // switches at end of frame; persisted in settings
ctx.window.size                 // real window pixels (read-only)
```

- `letterbox`: largest fit, bars. `integer`: whole multiples only, crisp pixel art, bars. `stretch`: fills, aspect lost. `expand`: fills with square pixels by showing more world (`ctx.window.size` grows the view). `crop`: fills keeping aspect, edges cut.
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
