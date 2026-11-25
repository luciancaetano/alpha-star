# Sprites, camera, tilemaps

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
into atlas pages. `pixelMode` (config or per asset) picks crisp or smooth filtering.

## Camera

```ts
ctx.camera.follow(hero, { lerp: 0.15, deadzone: [48, 32] });
ctx.camera.bounds([0, 0, map.size[0], map.size[1]]);
ctx.camera.shake(3, 0.25);
ctx.camera.zoom = 2;
ctx.camera.toWorld(ctx.input.mouse());
```

## Tiled maps

Save maps from [Tiled](https://www.mapeditor.org) as JSON (`.tmj`, CSV tile layers, tilesets as
`.tsj` or embedded, not infinite).

```ts
assets: { maps: { level1: "assets/maps/level1.tmj" } }
const map = ctx.spawnMap("level1", { collide: ["walls"] });   // solid layers -> merged static boxes tagged "walls"
for (const o of map.objects("spawns")) if (o.type === "coin") ctx.spawn(coin(o));
ctx.map("level1").properties;   // data without spawning
```

See `examples/platformer`.
