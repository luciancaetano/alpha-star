# Particles and effects

**What you'll build:** a ship with a flame trail flying over a scrolling starfield; Space sets
off an explosion (a particle burst, screen shake, a floating "+10" and a blink), a gem pulses
forever, and a label stays fixed on screen.

Effects are data: particles come from an `emitter` on a spawn spec, layers from `config.layers`,
motion from tweens. All of it runs on the fixed step with the game's seed, so it is the same on
every run. Replace `src/main.ts` with this and run `npm start`:

```ts
import { define, type Vec2 } from "alpha";

interface Fx {
  actions: "boom";
  axes: "moveX" | "moveY";
  sounds: never;
  images: never;
  tags: { ship: {}; star: { speed: number }; blast: {}; label: {}; gem: {} };
  layers: "stars" | "fx" | "hud";
  ui: {};
  scenes: { play: undefined };
}

const { scene, game } = define<Fx>();

const play = scene("play", {
  enter(ctx) {
    // A starfield on a far layer: it moves at 30% of the camera (parallax).
    for (let i = 0; i < 60; i++) {
      ctx.spawn({ tags: ["star"], layer: "stars", pos: [ctx.random.range(-40, 360), ctx.random.range(0, 180)], shape: { rect: [1, 1], color: "#ffffff" }, data: { speed: ctx.random.range(10, 40) } });
    }
    // The ship carries a thruster: particles stream out behind it, forever.
    ctx.spawn({
      tags: ["ship"], pos: [80, 90], shape: { rect: [14, 8], color: "#29adff" },
      emitter: { rate: 60, lifetime: [0.2, 0.4], speed: [40, 80], angle: [170, 190], size: [3, 0], color: ["#ffa300ff", "#ff004d00"], blend: "add" },
    });
    // A gem that pulses forever (tween with yoyo + repeat).
    const gem = ctx.spawn({ tags: ["gem"], pos: [240, 90], shape: { circle: 6, color: "#00e436" } });
    ctx.tween(gem, { to: { scale: 1.5, alpha: 0.6 }, seconds: 0.5, ease: "sineInOut", yoyo: true, repeat: Infinity });
    // Screen-fixed text on a parallax-0 layer: never moves or shakes with the camera.
    ctx.spawn({ tags: ["label"], layer: "hud", pos: [160, 10], shape: { text: "SPACE: boom", size: 8 } });
  },

  update(ctx) {
    const ship = ctx.find("ship");
    ship.pos.x += ctx.input.axis("moveX") * 90 * ctx.dt;
    ship.pos.y += ctx.input.axis("moveY") * 90 * ctx.dt;

    // Many entities moved at once: bulk positions are much cheaper than entity.pos per star.
    const stars = ctx.positions("star");
    for (let i = 0; i < stars.count; i++) {
      stars.x[i] -= 30 * ctx.dt;
      if (stars.x[i] < -40) stars.x[i] += 400;
    }
    stars.write();

    if (ctx.input.pressed("boom")) {
      const at: Vec2 = [ship.pos.x + 30, ship.pos.y];
      // One-shot burst: the entity goes away with its last particle.
      ctx.spawn({
        tags: ["blast"], layer: "fx", pos: at,
        emitter: { burst: 40, oneShot: true, lifetime: [0.3, 0.7], speed: [40, 160], size: [4, 1], gravity: [0, 120], color: ["#ffec27ff", "#ff004d00"], blend: "add" },
      });
      ctx.camera.shake(4, 0.3);
      // "+10" floats up and fades, then is destroyed.
      const label = ctx.spawn({ tags: ["label"], layer: "fx", pos: at, shape: { text: "+10", size: 8, color: "#ffec27" } });
      ctx.tween(label, { to: { y: at[1] - 24, alpha: 0 }, seconds: 0.6, ease: "quadOut", onDone: () => ctx.destroy(label) });
      // Blink the ship for a moment.
      ship.alpha = 0.3;
      ctx.after(0.08, () => { ship.alpha = 1; });
    }
  },
});

export default game({
  config: {
    title: "Effects",
    size: [320, 180],
    background: "#0b0d1a",
    seed: 1,
    actions: { boom: ["Space", "GamepadA"] },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"], moveY: ["GamepadLeftY", "W/S", "Up/Down"] },
    layers: { stars: { z: -100, parallax: 0.3 }, fx: { z: 100 }, hud: { z: 200, parallax: 0 } },
  },
  start: "play",
  scenes: { play },
  assets: { sounds: {} },
});
```

Expected: an orange-to-red flame trails the ship as it moves; stars drift left; Space throws a
yellow burst that falls and fades, shakes the screen and floats a "+10"; the green gem breathes;
"SPACE: boom" stays put even while the screen shakes.

## Step by step

### An emitter

```ts
ctx.spawn({
  tags: ["torch"], pos: [100, 120],
  emitter: {
    rate: 30,                                // particles per second (0 = bursts only)
    max: 200,                                // most live particles of this emitter (default 100)
    lifetime: [0.5, 1],                      // seconds, random in [min, max]
    speed: [10, 30],                         // px/s
    angle: [250, 290],                       // degrees: 0 right, 90 down, 270 up (default all ways)
    gravity: [0, -40],                       // px/s²: negative y rises
    size: [6, 1],                            // at birth -> at death
    color: ["#ffa300ff", "#ff004d00"],       // at birth -> at death (fade out with alpha 00)
    shape: "rect",                           // "circle" (default) | "rect" | { image } | { sheet, frame }
    blend: "add",                            // glows where particles overlap
    smooth: true,                            // antialiased edges (default: !config.pixelMode)
  },
});
```

Expected: a flickering flame rising from the torch. Every `Span` field takes one number or
`[min, max]`. Particles are born at the entity (they follow it while being born) and then fly on
their own in world space.

### Image and sheet particles

```ts
ctx.spawn({ tags: ["smoke"], pos: [140, 120], emitter: { rate: 8, lifetime: 2, speed: 15, angle: 270, size: [4, 16], color: ["#c2c3c780", "#c2c3c700"], shape: { image: "spark" } } });
ctx.spawn({ tags: ["smoke"], pos: [180, 120], emitter: { burst: 6, oneShot: true, shape: { sheet: "fx", frame: 2 }, speed: [20, 40] } });
```

Images and sheet frames are tinted by `color`; they follow their image's `filter`.

### Bursts, one-shots and control

```ts
// one-shot: `burst` particles on the first step, then the entity removes itself
ctx.spawn({ tags: ["dust"], pos: feet, emitter: { burst: 10, oneShot: true, speed: [20, 50] } });

const torch = ctx.find("torch");
torch.emit(20);            // a burst on the next step (any emitter)
torch.emitting = false;    // stop `rate` emission (bursts still work)
torch.emitting = true;
torch.particles;           // live particles of this emitter
```

At most 20000 live particles in a scene. `ctx.settings.reducedMotion` scales counts to 1/4.

### Tweens

```ts
const door = ctx.find("door");
const t = ctx.tween(door, { to: { y: door.pos.y - 32 }, seconds: 1, ease: "cubicOut", delay: 0.5 });
t.done; t.active; t.cancel();

ctx.tween(door, { to: { rotation: 360 }, seconds: 2, repeat: Infinity });          // spin forever
ctx.tween(gem, { to: { scale: 1.5 }, seconds: 0.5, yoyo: true, repeat: Infinity }); // pulse
ctx.tween(coin, { to: { scale: 2.5, alpha: 0 }, seconds: 0.25, onDone: () => ctx.destroy(coin) });
```

An entity tweens `x`, `y`, `rotation`, `alpha` and `scale`. Values are read when the tween
starts (after `delay`). `yoyo` goes back to the start; `repeat` adds cycles (`Infinity` = forever).
Eases: `linear`, or `quad | cubic | quart | quint | sine | expo | circ | back | elastic | bounce`
followed by `In`, `Out` or `InOut` (`"backOut"`, `"bounceOut"`, `"elasticInOut"`).

A plain object tweens its number keys, e.g. a score that counts up:

```ts
const counter = { value: 0 };
ctx.tween(counter, { to: { value: 500 }, seconds: 1, ease: "expoOut" });
ctx.every(0.05, () => { ctx.find("label").text = String(Math.round(counter.value)); });
```

A tweened kinematic body moves by velocity, so riders are carried ([Physics](physics.md)).

### Looks

```ts
const e = ctx.find("ship");
e.tint = "#ff000080";   // multiplies the color ("#rrggbbaa"); white = unchanged
e.alpha = 0.5;          // 0..1
e.blend = "add";        // "alpha" (default) | "add" (glow) | "multiply" (shade)
e.scale = 2;            // > 0; text scales its size
e.flipX = true;         // images and sheets
e.z = 5;                // order inside its layer
```

The same `tint`, `alpha`, `blend` can be in the spec's `shape` or `emitter`. On an entity with an
emitter and no shape they apply to its particles.

### Layers and parallax

```ts
// types: layers: "far" | "fx" | "hud";
config: { layers: { far: { z: -100, parallax: 0.2 }, fx: { z: 100 }, hud: { z: 200, parallax: 0 } } }
```

```ts
ctx.spawn({ tags: ["hill"], layer: "far", pos: [100, 60], shape: { circle: 40, color: "#5f9e6e" } });
ctx.spawn({ tags: ["label"], layer: "hud", pos: [8, 8], shape: { text: "HP 3", size: 8, align: "left" } });
```

Layers draw in `z` order, then entities by their own `z`. `parallax` 1 (default) moves with the
world, 0.2 moves at a fifth of the camera (far scenery), 0 is fixed to the screen (HUD text,
full-screen flashes). Entities without `layer` are on the unnamed layer (z 0, parallax 1).

### Screen shake

```ts
ctx.camera.shake(3, 0.2);   // up to 3 px, fading over 0.2 s
```

Parallax-0 layers and the UI do not shake.

### Many moving things

```ts
const stars = ctx.positions("star");
for (let i = 0; i < stars.count; i++) stars.y[i] += 40 * ctx.dt;
stars.write();
```

Several times cheaper than `entity.pos` per entity when hundreds move every step
(`examples/shmup`).

### Lights and full-screen effects

```ts
// config: { lighting: { ambient: "#202040" } }
ctx.spawn({ tags: ["torch"], pos: [100, 120], light: { radius: 80, color: "#ffa300", intensity: 1.2 } });
ctx.postfx.set("bloom", { strength: 0.8 });
ctx.postfx.set("vignette");
ctx.postfx.remove("bloom");
```

See [Shaders and lighting](shaders.md) for every effect and custom shaders.

## Recipes

### Landing dust

```ts
function dust(feet: Vec2): Spec {
  return {
    tags: ["dust"], pos: feet, layer: "fx",
    emitter: { burst: 10, oneShot: true, lifetime: [0.3, 0.5], speed: [20, 50], angle: [190, 350],
               gravity: [0, 60], size: [5, 2], color: ["#b89a6cff", "#b89a6c00"], shape: "rect" },
  };
}
// when the hero lands: ctx.spawn(dust([hero.pos.x, hero.pos.y + 8]));
```

### Bullet with a trail

```ts
function shotWithTrail(from: Vec2): Spec {
  return {
    tags: ["shot"], pos: from, shape: { circle: 2, color: "#ffec27" },
    emitter: { rate: 80, lifetime: 0.2, size: [2, 0], color: ["#ffec27ff", "#ffa30000"] },
  };
}
```

Destroying the shot removes its particles too (they belong to its entity). For smoke that
outlives the shot, spawn a one-shot puff where it died.

### Death: spin, shrink, fade, then remove

```ts
ctx.tween(e, { to: { alpha: 0, scale: 0.2, rotation: 180 }, seconds: 0.4, ease: "quadIn", onDone: () => ctx.destroy(e) });
```

### Floating damage numbers

```ts
function damageNumber(ctx: Play, at: Vec2, amount: number) {
  const n = ctx.spawn({ tags: ["label"], layer: "fx", pos: at, shape: { text: String(amount), size: 8, color: "#ff004d" } });
  ctx.tween(n, { to: { y: at[1] - 20 }, seconds: 0.5, ease: "quadOut" });
  ctx.tween(n, { to: { alpha: 0 }, seconds: 0.2, delay: 0.4, onDone: () => ctx.destroy(n) });
}
```

### Screen flash

```ts
const [x, y, w, h] = ctx.camera.view;
const f = ctx.spawn({ tags: ["flash"], layer: "hud", pos: [x + w / 2, y + h / 2], shape: { rect: [w, h], color: "#ffffff" } });
ctx.tween(f, { to: { alpha: 0 }, seconds: 0.25, onDone: () => ctx.destroy(f) });
```

### Coin pop and idle bob

```ts
ctx.tween(coin, { to: { y: coin.pos.y - 4 }, seconds: 0.6, ease: "sineInOut", yoyo: true, repeat: Infinity });  // idle
ctx.tween(coin, { to: { scale: 2.5, alpha: 0 }, seconds: 0.25, ease: "quadOut", onDone: () => ctx.destroy(coin) }); // picked
```

### Hurt feedback

```ts
ctx.camera.shake(3, 0.2);
ctx.input.rumble(0.5, 0.15);
hero.alpha = 0.3;
ctx.after(0.1, () => { if (hero.alive) hero.alpha = 1; });
```

## Gotchas

- `tint` multiplies: it can darken or color, never brighten; blink with `alpha` or glow with `blend: "add"`.
- Change an emitter's numbers by spawning a new entity: the spec is read once (`emit`, `emitting` are live).
- Destroying an entity removes its live particles at once; a one-shot entity removes itself after its last one.
- Use `ctx.random` (seeded), never `Math.random` (disabled), for varied effects.
- Tests can count particles: `run.expect.particles().atLeast(5)`.

## See also

- [Sprites, camera, LDtk](tilemaps.md): camera follow, bounds and zoom.
- [Shaders and lighting](shaders.md): bloom, CRT, vignette, lights, custom shaders.
- [Text](text.md): world text and fonts.
- `examples/shmup` (starfield, explosions, CRT), `examples/platformer` (dust, coin pops, parallax hills).
