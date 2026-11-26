# Sprites, camera, LDtk levels

**What you'll build:** an animated hero (idle / run / jump / fall from a sprite sheet) that runs
through a level three screens wide, with spinning coins, a still image, a camera that follows
smoothly, stays inside the level, shakes on pickups and zooms on Z. Then the same hero in a level
made with [LDtk](https://ldtk.io).

Put the art next to your code: `assets/img/hero.png` (7 frames of 16x16 in one row),
`assets/img/coin.png` (4 frames of 8x8) and any 16x16 `assets/img/npc.png`. The first two are in
`examples/platformer/assets/img/`. Then replace `src/main.ts` and run `npm start`:

```ts
import { define } from "alpha";

interface Sprites {
  actions: "jump" | "zoom";
  axes: "moveX";
  sounds: never;
  images: "npc";
  tags: { hero: {}; ground: {}; coin: {}; npc: {} };
  sheets: { hero: "idle" | "run" | "jump" | "fall"; coin: "spin" };
  ui: {};
  scenes: { play: undefined };
}

const { scene, game } = define<Sprites>();
const WORLD = { width: 960, height: 180 };

const play = scene("play", {
  enter(ctx) {
    ctx.spawn({ tags: ["ground"], pos: [WORLD.width / 2, 172], shape: { rect: [WORLD.width, 16], color: "#5f574f" }, body: { type: "static", collider: { rect: [WORLD.width, 16] } } });
    // A sheet sprite: starts playing "idle".
    const hero = ctx.spawn({ tags: ["hero"], pos: [40, 140], shape: { sheet: "hero", animation: "idle" }, body: { type: "dynamic", fixedRotation: true, collider: { rect: [12, 16], friction: 0 } } });
    // Sheet sprites drawn bigger than their frame, and a plain image.
    for (let x = 120; x < WORLD.width; x += 80) ctx.spawn({ tags: ["coin"], pos: [x, 150], shape: { sheet: "coin", animation: "spin", size: [12, 12] } });
    ctx.spawn({ tags: ["npc"], pos: [300, 148], shape: { image: "npc" } });

    // The camera follows the hero smoothly and never shows outside the level.
    ctx.camera.follow(hero, { lerp: 0.15, deadzone: [40, 30] });
    ctx.camera.bounds([0, 0, WORLD.width, WORLD.height]);
  },

  update(ctx) {
    const hero = ctx.find("hero");
    const move = ctx.input.axis("moveX");
    let vy = hero.body.velocity.y;
    const onGround = Math.abs(vy) < 1 && hero.pos.y > 150;
    if (ctx.input.pressed("jump") && onGround) vy = -300;
    hero.body.velocity = [move * 100, vy];

    if (move !== 0) hero.flipX = move < 0;            // face where it walks
    if (!onGround) hero.animation = vy < 0 ? "jump" : "fall";
    else hero.animation = move !== 0 ? "run" : "idle"; // same name again = keeps playing

    if (ctx.input.pressed("zoom")) ctx.camera.zoom = ctx.camera.zoom === 1 ? 2 : 1;
    for (const c of ctx.findAll("coin")) {
      if (Math.abs(c.pos.x - hero.pos.x) < 10 && Math.abs(c.pos.y - hero.pos.y) < 16) {
        ctx.destroy(c);
        ctx.camera.shake(2, 0.15);
      }
    }
  },
});

export default game({
  config: {
    title: "Sprites",
    size: [320, 180],
    background: "#87ceeb",
    seed: 1,
    gravity: [0, 900],
    actions: { jump: ["Space", "W", "Up", "GamepadA"], zoom: ["Z", "GamepadY"] },
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"] },
  },
  start: "play",
  scenes: { play },
  assets: {
    sounds: {},
    images: { npc: "assets/img/npc.png" },
    // hero.png: 7 frames of 16x16 in a row; coin.png: 4 frames of 8x8.
    sheets: {
      hero: {
        image: "assets/img/hero.png",
        frame: [16, 16],
        animations: {
          idle: { frames: [0] },
          run: { frames: [1, 2, 3, 4], fps: 12 },
          jump: { frames: [5] },
          fall: { frames: [6] },
        },
      },
      coin: { image: "assets/img/coin.png", frame: [8, 8], animations: { spin: { frames: [0, 1, 2, 3], fps: 8 } } },
    },
  },
});
```

Expected: the hero stands idle, runs (facing the way it goes) with the arrows, shows the jump
frame going up and the fall frame coming down; coins spin; the camera trails the hero, stops at
the level's ends, shakes a little per coin and doubles in size with Z.

## Step by step

### Images

```ts
// types: images: "npc" | "sky";
assets: { images: {
  npc: "assets/img/npc.png",
  sky: { file: "assets/img/sky.png", filter: "linear" },   // smooth art; "nearest" crisp; "sharp" pixel art at any scale/angle
} }
```

```ts
ctx.spawn({ tags: ["npc"], pos: [300, 148], shape: { image: "npc" } });                   // its own pixel size
ctx.spawn({ tags: ["sky"], pos: [160, 90], shape: { image: "sky", size: [320, 180] }, z: -10 });
```

Expected: the image is centered on `pos`. Without `filter`, `config.pixelMode` decides
(default `true`: crisp). `alpha export` packs every PNG into atlas pages.

### Sprite sheets and animations

```ts
// types: sheets: { hero: "idle" | "run" | "jump" | "fall" };
sheets: {
  hero: {
    image: "assets/img/hero.png",
    frame: [16, 16],                           // cell size; frame 0 is top-left, row-major
    animations: {
      idle: { frames: [0] },
      run: { frames: [1, 2, 3, 4], fps: 12 },  // fps default 10
      jump: { frames: [5], loop: false },      // a one-shot holds its last frame
    },
  },
}
```

```ts
const hero = ctx.spawn({ tags: ["hero"], pos: [40, 140], shape: { sheet: "hero", animation: "idle" } });
hero.animation = "run";   // switch (from its first frame); setting the same name keeps it going
hero.play("run");         // restart even if already playing
hero.animation;           // "run" ("" before any)
hero.flipX = true;        // face left
```

`size: [w, h]` draws a frame at another size; `flipX` / `flipY` can be in the spec too. A sheet
whose animations are one frame each is a tile set (`examples/puzzle`). An unknown animation name
is an error that lists the sheet's animations.

### Look of a sprite

```ts
hero.tint = "#ff8080";   // multiply color ("#rrggbbaa" works)
hero.alpha = 0.5;
hero.scale = 2;
hero.rotation = 15;
```

More in [Particles and effects](particles.md#looks).

### Camera follow

```ts
ctx.camera.follow(hero);                                   // locked on
ctx.camera.follow(hero, { lerp: 0.15, deadzone: [40, 30] }); // smooth; free box in the middle
ctx.camera.follow(null);                                   // stop following
```

`lerp` is the fraction of the remaining distance covered per step (0..1]. Inside the `deadzone`
box the hero moves without moving the camera.

### Camera bounds, zoom, rotation, position

```ts
ctx.camera.bounds([0, 0, 960, 180]);   // never show outside this rect; null removes it
ctx.camera.zoom = 2;                   // everything twice as big
ctx.camera.rotation = 5;               // degrees
ctx.camera.pos.x = 480;                // world point at the screen center (when not following)
const [vx, vy, vw, vh] = ctx.camera.view;   // the world rect visible right now
```

Each scene starts with the camera centered on `config.size`, zoom 1. Read `ctx.camera.view` each
time you need the visible area: it changes with zoom, movement and window size.

### Screen shake

```ts
ctx.camera.shake(4, 0.3);   // up to 4 px, fading out over 0.3 s
```

Deterministic, and scaled down when the player turns on `ctx.settings.reducedMotion`.

### Mouse to world

```ts
const world = ctx.camera.toWorld(ctx.input.mouse());   // where the pointer is in the level
```

## LDtk levels

Draw levels in [LDtk](https://ldtk.io) (1.0 or newer, one world per project) and save the `.ldtk`
project inside the game folder; its tileset images and external levels (`.ldtkl`) come along on
export. This is the hero from above in the `examples/platformer` world (copy
`examples/platformer/assets/levels/` and `assets/tiles/`): an IntGrid layer `Walls`, tile layers,
and `Player` / `Coin` entities.

```ts
import { define, type EntitySpec, type LevelEntity } from "alpha";

interface Levels {
  actions: "jump";
  axes: "moveX";
  sounds: never;
  images: never;
  tags: { player: {}; coin: {}; walls: {} };
  sheets: { hero: "idle" | "run"; coin: "spin" };
  worlds: "main";
  ui: {};
  scenes: { boot: undefined; play: { level: string } };
}

const { scene, game } = define<Levels>();

// Factories: one LDtk entity -> a spawn spec (or several, or null to skip it).
const player = (e: LevelEntity): EntitySpec<Levels, "player"> => ({
  tags: ["player"], pos: e.pos, shape: { sheet: "hero", animation: "idle" },
  body: { type: "dynamic", fixedRotation: true, collider: { rect: [12, 16], friction: 0 } },
});
const coin = (e: LevelEntity): EntitySpec<Levels, "coin"> => ({
  tags: ["coin"], pos: e.pos, shape: { sheet: "coin", animation: "spin" },
  body: { type: "static", collider: { circle: 4, sensor: true } },
});

const boot = scene("boot", { enter: (ctx) => ctx.go("play", { level: "Level_0" }) });

const play = scene("play", {
  enter(ctx, { level: id }) {
    const level = ctx.spawnLevel("main", id, {
      collide: { Walls: "walls" },                  // IntGrid layer -> merged static colliders tagged "walls"
      entities: { Player: player, Coin: coin },     // LDtk entity identifier -> factory
      background: true,                             // fill the level's LDtk background color
    });
    ctx.camera.follow(ctx.find("player"), { lerp: 0.15 });
    ctx.camera.bounds([level.pos[0], level.pos[1], level.size[0], level.size[1]]);
    ctx.onEnter("player", "coin", (_p, c) => ctx.destroy(c));
  },
  update(ctx) {
    const p = ctx.find("player");
    p.body.velocity = [ctx.input.axis("moveX") * 100, p.body.velocity.y];
  },
});

export default game({
  config: {
    title: "Levels",
    size: [320, 180],
    seed: 1,
    gravity: [0, 900],
    actions: { jump: ["Space"] },
    axes: { moveX: ["A/D", "Left/Right", "GamepadLeftX"] },
  },
  start: "boot",
  scenes: { boot, play },
  assets: {
    sounds: {},
    worlds: { main: "assets/levels/world.ldtk" },   // or { file, filter } for its tilesets
    sheets: {
      hero: { image: "assets/img/hero.png", frame: [16, 16], animations: { idle: { frames: [0] }, run: { frames: [1, 2, 3, 4], fps: 12 } } },
      coin: { image: "assets/img/coin.png", frame: [8, 8], animations: { spin: { frames: [0, 1, 2, 3], fps: 8 } } },
    },
  },
});
```

Expected: the level's tiles draw, the hero lands on the `Walls` cells, eight coins spin where
they were placed in LDtk and vanish when touched, and the camera stays inside the level.

### Colliders from IntGrid

```ts
collide: { Walls: "walls" }   // every non-zero value -> static colliders tagged "walls"
// or one rule per IntGrid value (its identifier or number):
collide: { Walls: { ground: "walls", brick: { tag: "ledge", oneWay: true } } }
```

A rule takes `tag` plus `oneWay`, `sensor`, `friction`, `restitution`. Neighbouring cells merge
into a few big static boxes, so the tag works with `onHit`, `raycast` and queries like any body.

### Entities and their fields

```ts
entities: {
  Player: player,
  Coin: (e) => (e.fields.hidden === true ? null : coin(e)),   // skip some
  Door: (e) => ({ tags: ["door"], pos: e.pos, body: { type: "static", collider: { rect: e.size, sensor: true } },
                  data: { to: e.fields.to as string } }),
}
```

A factory gets a `LevelEntity`: `id` ("Coin"), `iid`, `pos` (world pixels of its pivot), `local`,
`grid` (cell), `size`, `pivot`, `layer`, `tags`, `tile` and `fields`. Field values are plain data:
numbers, strings (enums and colors `"#rrggbb"` too), booleans, null, Point as `[cx, cy]`, Tile as
`{ tileset, rect }`, arrays, and EntityRef as `{ entity, layer, level }` iids:

```ts
const ref = level.entities("Lever")[0].fields.door;
if (ref && typeof ref === "object" && "entity" in ref) level.entity(ref.entity);   // the spawned door
```

### Reading and editing cells

```ts
level.int("Walls", 3, 7);                          // IntGrid value (0 = empty or outside)
level.tile("Ground", 3, 7);                        // { id, flipX, flipY, tags, data } or null
const cell = level.toCell([100, 120], "Walls");    // world point -> [cx, cy]
level.toWorld(cell, "Walls");                      // cell center in world pixels
level.setInt("Walls", cell[0], cell[1], 0);        // clear: that layer's colliders rebuild
level.setTile("Ground", cell[0], cell[1], null);   // and its tile, redrawn in place
level.setInt("Walls", 5, 5, "brick");              // by identifier
level.setTile("Ground", 5, 5, 2, { flipX: true }); // tile id 2 of the layer's tileset
level.entities("Coin");                            // copies of the LDtk instances
level.destroy();                                   // tiles, colliders, background and factory entities
```

Auto-layer tiles are not re-run by `setInt`: change the look with `setTile`.

### Drawing order and parallax

```ts
// types: layers: "far";  config: { layers: { far: { z: -100, parallax: 0.3 } } }
ctx.spawnLevel("main", "Level_0", { layers: { Deco: "far" }, z: -1 });
```

Each tile layer is one batched, culled entity; LDtk flips and opacity apply. `z` places every tile
layer, `layer` puts all of them on a `config.layers` layer, `layers` maps single LDtk layers (e.g.
a parallax backdrop). Animate a tile with tileset custom data `{"frames": [4, 5], "fps": 2}`.
Background images, IntGrid colors and LDtk's own parallax factors are not drawn.

### World data without spawning

```ts
const world = ctx.world("main");
world.levels;                      // ["Level_0", ...] in project order
world.layout;                      // "Free" | "GridVania" | "LinearHorizontal" | "LinearVertical"
world.level("Level_0").size;       // any level's data: entities, int, tile, fields, neighbours
world.levelAt([500, 100]);         // the level under a world point, or null
world.entity(iid);                 // an instance anywhere, with its level id
```

## Recipes

### Room transitions (rooms streamed as you walk)

```ts
// state: () => ({ room: null as Level<MyGame["tags"]> | null })
function enterRoom(ctx: Play, id: string) {
  ctx.state.room?.destroy();
  const room = ctx.spawnLevel("main", id, { collide: { Walls: "walls" }, entities: { Coin: coin } });
  ctx.state.room = room;
  ctx.camera.bounds([room.pos[0], room.pos[1], room.size[0], room.size[1]]);
}

// enter: the hero lives outside any room, so destroying a room keeps it
const start = ctx.world("main").level("Level_0").entities("Player")[0];
ctx.camera.follow(ctx.spawn(player(start)), { lerp: 0.2 });
enterRoom(ctx, "Level_0");

// update:
const p = ctx.find("player");
const here = ctx.world("main").levelAt([p.pos.x, p.pos.y]);
if (here && here.id !== ctx.state.room?.id) enterRoom(ctx, here.id);
```

Levels spawn at their LDtk world position (Free / GridVania; Linear layouts end to end), so
neighbours line up. `level.neighbours` lists adjacent level ids and directions.

### Door to another level (scene restart)

```ts
ctx.onEnter("player", "door", (_p, d) => ctx.go("play", { level: d.data.to as string }));
```

### Dig through bricks

```ts
if (ctx.input.pressed("dig")) {
  const [cx, cy] = level.toCell([p.pos.x, p.pos.y + 16], "Walls");
  if (level.int("Walls", cx, cy) === 2) {       // 2 = "brick"
    level.setInt("Walls", cx, cy, 0);
    level.setTile("Ground", cx, cy, null);
  }
}
```

### Facing and animation state in one place

```ts
type Hero = Entity<MyGame, "hero">;   // import type { Entity } from "alpha"; animations come from MyGame["sheets"]

function animate(hero: Hero, move: number, grounded: boolean) {
  if (move !== 0) hero.flipX = move < 0;
  if (!grounded) hero.animation = hero.body.velocity.y < 0 ? "jump" : "fall";
  else hero.animation = move !== 0 ? "run" : "idle";
}
```

Assigning every step is fine: the same name keeps the animation going.

### Look-ahead camera

```ts
ctx.camera.follow(null);
const target = hero.pos.x + (hero.flipX ? -40 : 40);
ctx.camera.pos.x += (target - ctx.camera.pos.x) * 0.1;
ctx.camera.pos.y = hero.pos.y;
```

## Gotchas

- `size` on a sheet or image changes how big it draws, not its collider.
- Set an animation every step if you like; use `play()` only to restart one.
- Read `ctx.camera.view` instead of assuming `config.size`: phones and resized windows show more.
- Factory entities belong to the level: `level.destroy()` removes those still alive.
- Not supported (clear error): multi-world LDtk projects, LDtk older than 1.0, tilesets without an image.

## See also

- [Physics](physics.md): ground checks, one-way ledges, sensors for coins and doors.
- [Particles and effects](particles.md): layers, parallax, tweens, shake.
- [Concepts](concepts.md#later-topics): image filtering and window stretch.
- `examples/platformer` (sheets, LDtk, camera, lift), `examples/puzzle` (sheet as tile set).
