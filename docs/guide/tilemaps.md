# Sprites, camera, LDtk levels

## Sprite sheets

```ts
// types.ts
sheets: { hero: "idle" | "run" | "jump" };
// main.ts
assets: { sheets: { hero: { image: "assets/img/hero.png", frame: [16, 16],
  animations: { idle: { frames: [0] }, run: { frames: [1, 2, 3, 4], fps: 12 }, jump: { frames: [5], loop: false } } } } }
```

```ts
const hero = ctx.spawn({ tags: ["player"], pos, shape: { sheet: "hero", animation: "idle" } });
hero.animation = "run";   // switches; setting the playing name again keeps it going
hero.play("run");         // restarts
```

A sheet with one-frame animations is a tile set (`examples/puzzle`). `alpha export` packs images
into atlas pages. `config.pixelMode` picks the default filtering; a world overrides it for its tileset images with `{ file, filter: "nearest" | "linear" | "sharp" }`.

## Camera

```ts
ctx.camera.follow(hero, { lerp: 0.15, deadzone: [48, 32] });
ctx.camera.bounds([0, 0, level.size[0], level.size[1]]);
ctx.camera.shake(3, 0.25);
ctx.camera.zoom = 2;
ctx.camera.toWorld(ctx.input.mouse());
```

## LDtk levels

Make levels in [LDtk](https://ldtk.io) (1.0 or newer, one world per project) and save the `.ldtk`
project inside the game folder; tileset images and external levels (`.ldtkl`) come along on export.

```ts
// types.ts
worlds: "main";
// main.ts
assets: { worlds: { main: "assets/levels/world.ldtk" } }   // or { file, filter }
```

```ts
const level = ctx.spawnLevel("main", "Level_0", {
  collide: { Walls: "walls" },                          // IntGrid layer -> merged static boxes tagged "walls"
  entities: { Player: (e) => player(e.pos), Coin: coin }, // LDtk entity -> spawn spec(s) or null
});
level.int("Walls", 3, 7);            // IntGrid value (0 = empty or outside)
level.tile("Ground", 3, 7);          // { id, flipX, flipY, tags, data } or null
level.setInt("Walls", 3, 7, "brick"); // colliders of that layer rebuild
level.setTile("Ground", 3, 7, 2);    // redraws in place
ctx.world("main").levels;            // data only: level(id), levelAt(pos), entity(iid)
```

- **Colliders:** `collide` takes a tag for every non-zero value, or a rule per value / identifier:
  `{ Walls: { ground: "walls", platform: { tag: "ledge", oneWay: true } } }` (also `sensor`,
  `friction`, `restitution`). Cells merge into a few static boxes.
- **Entities:** each factory gets a `LevelEntity`: `pos` in world pixels (pivot point), `local`, `grid`,
  `size`, `pivot`, `tags` and typed `fields` (enums as strings, Color `"#rrggbb"`, Point `[cx, cy]`,
  EntityRef iids resolved with `level.entity(iid)`, FilePath resolved, Tile `{ tileset, rect }`).
- **Drawing:** each tile layer is one batched, culled entity; flips and opacity follow LDtk. Animate a
  tile with tileset custom data `{"frames": [4, 5], "fps": 2}`. `z`, `layer` and `layers`
  (per LDtk layer, e.g. a parallax `config.layers` entry) place them; `background: true` fills the
  level color. Background images, IntGrid colors and LDtk parallax factors are not drawn.
- **Several levels:** `pos` defaults to the level's LDtk world position (Free / GridVania; Linear
  layouts end to end); use `neighbours` and `world.levelAt(pos)` to stream them.
- Not supported (clear error): multi-world projects, LDtk older than 1.0, tilesets without an image.

See `examples/platformer`.
