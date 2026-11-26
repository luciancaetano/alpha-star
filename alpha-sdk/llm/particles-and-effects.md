# Particles and Effects

> summary: Particle emitters on spawn specs, looks (tint/alpha/blend), 2D lighting (config.lighting + entity light) and full-screen post effects (ctx.postfx); read when adding visual effects.

Effects are data: particles only through a spawn spec's `emitter` (plus `entity.emit(n)`); layers only through `config.layers` + `layer:`.

## Emitter API

```ts
interface EmitterSpec {          // also takes tint?, alpha?, blend?
  rate?: number;                 // per second (default 0: bursts only)
  burst?: number;                // on the first step (default 0)
  lifetime?: Span;               // seconds (default 1); Span = number | [min, max]
  speed?: Span;                  // px/s (default 0)
  angle?: Span;                  // degrees, 0 right, 90 down, 270 up (default [0, 360])
  gravity?: Vec2;                // px/s²
  size?: Span;                   // [birth, death] px (default 4)
  color?: Color | [Color, Color];// default ["#ffffffff", "#ffffff00"] (fades out)
  shape?: "circle" | "rect" | { image } | { sheet, frame? };   // default circle; tinted by color
  smooth?: boolean;   // circle/rect: antialiased edges; default !config.pixelMode (images follow their filter)
  max?: number;                  // live particles of this emitter, 1..20000 (default 100)
  oneShot?: boolean;             // emit burst once; entity destroyed when the last particle dies
}
e.emit(n); e.emitting = false; e.particles   // burst later / stop `rate` / live count
```

## Example (examples/platformer/src/prefabs/effects.ts)

```ts
export function dust(feet: Vec2): Spec {
  return { tags: ["dust"], pos: feet, layer: "fx",
    emitter: { burst: DUST.count, oneShot: true, lifetime: [0.3, 0.5], speed: [20, 50], angle: [190, 350],
               gravity: [0, 60], size: [5, 2], color: ["#b89a6cff", "#b89a6c00"], shape: "rect" } };
}
// play.ts on landing
ctx.spawn(dust([p.pos.x, p.pos.y + 8]));
```

## Looks

```ts
e.tint = "#ff000080"; e.alpha = 0.5; e.blend = "add";   // "alpha" | "add" | "multiply"
```
Without a shape, `tint`/`alpha`/`blend` apply to the emitter's particles.

## Lighting

```ts
config: { lighting: { ambient: "#404858" } },            // color where no light reaches; off when absent
ctx.spawn({ tags: ["torch"], pos: [100, 80], light: { radius: 110, color: "#ffd9a0", intensity: 1.5 } });
```
`radius` 1..4096 world px, `intensity` 0..8 (default 1), `color` default "#ffffff". Lights add up, follow their entity, layer parallax and zoom. A light needs no shape. Without `config.lighting`, lights are ignored.

## Post effects

```ts
ctx.postfx.set("bloom", { strength: 0.9, radius: 3 });  // adds, or replaces params in place
ctx.postfx.set("crt");
ctx.postfx.set("crt", { strength: 0.8 });               // only if already added (else error)
ctx.postfx.remove("crt"); ctx.postfx.clear(); ctx.postfx.list();
```
| Effect | Params (default) |
|---|---|
| `bloom` | `strength` 0..2 (0.6), `radius` 1..16 (4) |
| `crt` | `strength` 0..1 (0.5) |
| `pixelate` | `size` 1..64 (4) |
| `tint` | `color` ("#ffffff"), `strength` 0..1 (1) |
| `vignette` | `strength` 0..1 (0.5), `radius` 0..1 (0.5) |

## Rules and gotchas

- Particles simulate on the fixed step with the game's seeded random: `run.expect.particles().equals(n)` is exact.
- At most 20000 live particles in the whole scene.
- `ctx.settings.reducedMotion` scales rate, burst, max and emit counts to 1/4.
- Lighting and postfx are render-only: tests and saves never see them. UI and dev tools draw on top, unaffected. Lighting is applied before postfx (bloom makes lit areas glow).
- Postfx draw in first-added order; each scene starts with none. Unknown names/params or out-of-range values pause with an error listing what is allowed (e.g. `postfx "crt": unknown param "amount"; have: strength`).
- Custom full-screen shaders: `assets.shaders` + the same `ctx.postfx` calls; see shaders.md.
- If a renderer cannot make render targets, lighting, bloom and pixelate are skipped with one notice.
