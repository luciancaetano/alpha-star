# Tilemaps (LDtk levels)

> summary: Loading LDtk worlds (.ldtk), spawnLevel with IntGrid colliders, entity factories, typed fields, editing cells, multi-level worlds; read when building levels.

## API

```ts
// types.ts
worlds: "main";
// main.ts assets (string or { file, filter } for its tileset images)
worlds: { main: "assets/levels/world.ldtk" },

ctx.world(name): World                              // data only, spawns nothing
ctx.spawnLevel(world, level, opts?): Level          // draws tiles, colliders, entities; call in enter
interface SpawnLevelOptions {
  collide?: { [intGridLayer]: Tag | { [valueOrIdentifier]: Tag | { tag, oneWay?, sensor?, friction?, restitution? } } };
  entities?: { [ldtkEntity]: (e: LevelEntity) => Spec | Spec[] | null };  // factory per identifier
  z?: number;          // z of every tile layer (default 0; later LDtk layers draw over earlier)
  pos?: Vec2;          // level top-left in world px (default its LDtk world position)
  layer?: string;      // config.layers layer of every tile layer
  layers?: { [ldtkLayer]: string };   // per LDtk layer override (e.g. parallax)
  background?: boolean;               // fill the LDtk level bg color (default false)
}
interface World { name; layout /* Free | GridVania | LinearHorizontal | LinearVertical */;
  levels: string[]; level(id): LevelData; levelAt(pos): LevelData | null; entity(iid) }
interface LevelData { id; iid; pos; size; depth; bg; fields; layers: string[]; neighbours;
  entities(id?): LevelEntity[]; int(layer, cx, cy): number; tile(layer, cx, cy): LevelTile | null;
  toCell(pos, layer?): Vec2; toWorld(cell, layer?): Vec2 }
interface Level extends LevelData {   // spawned
  setInt(layer, cx, cy, value /* number | identifier, 0 clears */); setTile(layer, cx, cy, tileId | null, { flipX?, flipY?, alpha? }?);
  entity(iid): Entity | null; destroy() }
interface LevelEntity { id; iid; pos /* world px of its pivot */; local; grid; size; pivot; layer; tags; fields; tile }
interface LevelTile { id; flipX; flipY; tags /* tileset enum tags */; data /* custom data string | null */ }
```

Fields are plain data: numbers, strings, booleans, null; enums as strings; Color `"#rrggbb"`;
Point `[cx, cy]`; EntityRef `{ entity, layer, level }` iids (`level.entity(ref.entity)`);
FilePath resolved to a pack path; Tile `{ tileset, rect }`; arrays of these.

## Example (examples/platformer/src/scenes/play.ts)

```ts
enter(ctx) {
  // IntGrid "Walls" (values ground, brick) -> merged static colliders tagged "walls" (a tag in types.ts)
  const level = ctx.spawnLevel("main", "Level_0", {
    collide: { Walls: "walls" },
    entities: { Player: (e) => player(e.pos), Coin: coin },   // coin(at: LevelEntity) uses at.pos
  });
  const hero = ctx.find("player");
  ctx.camera.bounds([0, 0, level.size[0], level.size[1]]);
  ctx.camera.follow(hero, { lerp: 0.15, deadzone: [48, 32] });
}
```

Per-value rules: `collide: { Walls: { ground: "walls", platform: { tag: "ledge", oneWay: true }, 3: { tag: "spikes", sensor: true } } }`
(values not listed are not solid).

## Rules and gotchas

- Scene-scoped like spawn: call `spawnLevel` in `enter`; `level.destroy()` removes everything it made.
- Each tile layer is one batched, culled entity (+1 for its animated tiles). Tiles keep LDtk flips
  and opacity; layer opacity applies.
- IntGrid cells merge into a few static boxes (`run.expect.count("walls").atLeast(n)` in tests).
- Animated tiles: tileset custom data on the first tile `{"frames": [4, 5], "fps": 2}`.
- `level.int(layer, cx, cy)` is 0 outside the layer; `tile()` returns the top tile or null.
- `setInt` rebuilds that layer's colliders but does not re-run auto-layer rules (change visuals with
  `setTile`); `setTile` redraws in place.
- Multi-level: `world.levelAt(pos)`, `neighbours`; Free/GridVania levels keep their LDtk position,
  Linear layouts are laid end to end. External levels (`.ldtkl`) work.
- Not drawn: level background images, IntGrid colors, LDtk layer parallax (use `layers` + `config.layers`).
- Not supported (clear error): multi-world projects, LDtk JSON older than 1.0, tilesets without an
  image file (LDtk internal icons).
- `alpha export` copies the `.ldtk`, its tileset images and `.ldtkl` files.

## Errors

- `spawnLevel("x"): not in assets.worlds; have: ...`
- `world "main" has no level "Nope"; it has: Level_0`
- `has no IntGrid layer "Deco"; it has: Walls`
- `has no IntGrid value "lava"; it has: 0, 1 (wall), 2 (oneway)`
- `no entity "Enemy" in the project; it has: ...`
- `cell 9, 0 is outside layer "Walls" (4 x 3 cells)`
- `multi-world projects are not supported; turn off "Multi-worlds"` in LDtk's project settings
