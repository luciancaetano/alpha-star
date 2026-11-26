# Particles, looks, layers

## Particles

Effects are data: an `emitter` on a spawn spec. Particles are born at the entity and fly on
their own, simulated on the fixed step with the game's seed.

```ts
ctx.spawn({ tags: ["dust"], pos: feet, layer: "fx",
  emitter: { burst: 10, oneShot: true, lifetime: [0.3, 0.5], speed: [20, 50], angle: [190, 350],
             gravity: [0, 60], size: [5, 2], color: ["#b89a6cff", "#b89a6c00"], shape: "rect", blend: "add" } });
trail.emit(8);           // burst on an entity spawned with an emitter
trail.emitting = false;  // stop `rate` emission
```

A one-shot emitter's entity goes away with its last particle. At most 20000 live particles per
scene. `ctx.settings.reducedMotion` scales counts to 1/4. Tests: `run.expect.particles().atLeast(5)`.

Circle and rect particles follow `config.pixelMode` like filled shapes: crisp by default, antialiased
(a 1-pixel feathered edge) with `pixelMode: false`. Override per emitter with `smooth: true | false`.
Image and sheet particles follow their image's `filter`. Smooth particles cost about 2.5x the
vertices of crisp ones (20000 live: ~10 ms vs ~4 ms render on the bench machine).

## Looks

```ts
e.tint = "#ff000080"; e.alpha = 0.5; e.blend = "add"; e.scale = 2; e.flipX = true;
```

## Layers

```ts
config: { layers: { far: { z: -100, parallax: 0.2 }, fx: { z: 100 } } }
ctx.spawn({ ...hill(x), layer: "far" });
```

Layers draw in `z` order (then entity `z`); parallax 0 is fixed to the screen, 1 moves with the world.

## Many entities

```ts
const stars = ctx.positions("star");
for (let i = 0; i < stars.count; i++) stars.y[i] += 40 * ctx.dt;
stars.write();
```

Several times cheaper than `entity.pos` per entity when thousands move every frame (`examples/shmup`).

## World text and the window

```ts
ctx.spawn({ tags: ["label"], pos, shape: { text: "+1", size: 8, color: "#ffe14d" } });  // built-in font without `font`
config: { window: { scale: "integer" }, pixelMode: true }   // aspect: expand (default) | keep_width | keep_height | keep
ctx.window.fullscreen = true;   // Alt+Enter too
```

Full-screen effects (bloom, CRT, ...) and 2D lights: see [Shaders and lighting](shaders.html).
