# Sprites and Images

> summary: Image assets, sprite sheets and animations, flips, texture filtering (filter nearest|linear|sharp, config.pixelMode) and atlas packing; read when drawing images or animated sprites.

## API

```ts
// types.ts
images: "logo" | "sky";                    // `never` if none
sheets: { hero: "idle" | "run" | "jump" }; // sheet -> animation names
// main.ts assets
images: { logo: "assets/img/logo.png", sky: { file: "assets/img/sky.png", filter: "linear" } },
sheets: { hero: { image: "assets/img/hero.png", frame: [16, 16], // filter?: ImageFilter
  animations: { idle: { frames: [0] }, run: { frames: [1, 2, 3, 4], fps: 12 }, jump: { frames: [5], loop: false } } } },

type ImageFilter = "nearest" | "linear" | "sharp";
type ImageAsset = string | { file: string; filter?: ImageFilter };
interface AnimationDef { frames: number[]; fps?: number /* 10 */; loop?: boolean /* true */ }

// shapes
{ image: "logo", size?: [w, h], flipX?, flipY? }                     // size defaults to the image's pixels
{ sheet: "hero", animation?: "run", size?: [w, h], flipX?, flipY? }  // starts at frame 0; size = frame size
// entity
e.animation = "run";   // switch; setting the playing name again keeps it going; "" before any
e.play("run");         // restart from the first frame
e.flipX = true;
```

Sheet grid: `frame`-sized cells, row-major, frame 0 top-left. A sheet of one-frame animations works as a tile set (`examples/puzzle`).

## Example (examples/platformer)

```ts
// prefab: shape: { sheet: "hero", animation: "idle" }
function animate(p: Hero, ctx: Play, move: number) {
  if (move !== 0) p.flipX = move < 0;
  if (!ctx.state.grounded) p.animation = p.body.velocity.y < 0 ? "jump" : "fall";
  else p.animation = move !== 0 ? "run" : "idle";
}
```

## Filtering

- `config.pixelMode` (default `true`): `true` = nearest filtering + hard shape edges; `false` = linear + antialiased circle/rect edges.
- Override per asset: `{ file, filter }` on images and maps (their tilesets), `filter` on sheets. Filled shapes: `smooth`. Fonts: `{ file, pixelMode }`.
- `"nearest"`: crisp; uneven/jagged at non-whole scales or rotated. `"linear"`: smooth art, photos (pixel art blurs). `"sharp"`: pixel art at any size or angle (square pixels, soft ~1 px edges); 16x that image's texture memory.
- Never put `pixelMode` on an image/sheet/map: the engine rejects it (`pixelMode is not an image option; use filter: ...`). Older README text shows that form; `filter` is current.
- Crisp pixel art also wants `config.window.scaling: "integer"` (camera-and-window.md).
- Particles with an image or sheet shape use their source's filtering.

## Rules and gotchas

- Unknown animation names throw, listing the sheet's animations.
- Animations advance on the fixed step (deterministic): `run.expect.animation("player").equals("run")`.
- `loop: false` holds the last frame.
- `alpha export` packs PNG images and sheets into `alpha-atlas-N.png` / `alpha-atlas-smooth-N.png` pages; transparent to game code. `--no-atlas` keeps separate files.
- Paths are relative to the project and must stay inside it (`file "x" not found (paths are relative to ...)`).
