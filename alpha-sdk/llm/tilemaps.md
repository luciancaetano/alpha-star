# Tilemaps

> summary: Loading Tiled (.tmj) maps, spawnMap with collider layers and draw layers, reading object layers as spawn points; read when building levels from Tiled.

## API

```ts
// types.ts
maps: "level1";
// main.ts assets (string or { file, filter } for its tilesets)
maps: { level1: "assets/maps/level1.tmj" },

ctx.spawnMap(name, opts?): TileMap  // draws tile layers, makes `collide` layers solid; call in enter
ctx.map(name): TileMap              // data only, spawns nothing
interface SpawnMapOptions {
  collide?: T[];                    // tile layers -> merged static rect colliders tagged with the layer name
  z?: number;                       // z of every tile layer (default 0)
  pos?: Vec2;                       // where the map's top-left goes (default [0, 0])
  layer?: string;                   // config.layers layer for every tile layer
  layers?: { [tiledLayer: string]: string };  // per Tiled layer override
}
interface TileMap { name; width; height /* tiles */; tileSize: Vec2; size: Vec2 /* px */;
  properties; layers: string[]; objects(layer): MapObject[] }
interface MapObject { id; name; type /* Tiled class */; x; y /* top-left, or the point */;
  width; height; rotation; point: boolean; properties }
```

## Example (examples/platformer/src/scenes/play.ts)

```ts
enter(ctx) {
  const map = ctx.spawnMap("level1", { collide: ["walls"] });   // "walls" must be a tag too
  const spawns = map.objects("spawns");
  const start = spawns.find((o) => o.name === "player") ?? spawns[0];
  const hero = ctx.spawn(player([start.x, start.y]));
  for (const o of spawns.filter((s) => s.type === "coin")) ctx.spawn(coin(o));
  ctx.camera.bounds([0, 0, map.size[0], map.size[1]]);
  ctx.camera.follow(hero, { lerp: 0.15, deadzone: [48, 32] });
}
```

## Tiled format rules

- Save maps as JSON `.tmj`, CSV tile layers, not infinite.
- Tilesets embedded or external JSON `.tsj`; `.tsx` is rejected ("save tilesets as JSON (.tsj) in Tiled").
- Referenced tilesets/images must be inside the project; `alpha export` copies them with the map.

## Rules and gotchas

- Collider tiles merge into a few static boxes (`run.expect.count("walls").atLeast(n)` in tests).
- Each tile layer becomes one batched, culled entity per layer and tileset.
- `objects(layer)` returns a copy; an unknown layer throws listing the object layers.
- Unknown map name: `spawnMap("x"): not in assets.maps; have: ...`.
- Scene-scoped like spawn: call `spawnMap` in `enter`.
- Parallax tile layers: `layers: { bg: "far" }` with `config.layers.far.parallax`.
