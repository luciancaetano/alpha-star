# Examples

Six complete games live in the repository's `examples/` folder. Each is small, has headless tests
and an `AGENTS.md`, and teaches a different part of the engine. Five of them are `alpha init`
templates, so you can start your own game from one in a minute.

## Play one now

```sh
npx @alpha-star/sdk init my-platformer --template platformer
cd my-platformer
npm install
npm start
```

**Expected:** a small platform game. Move with Left/Right or A/D, jump with Space, restart with
R. Then run `npm test` to see the tests that keep it working.

Templates: `blank` (the starter), `platformer`, `topdown`, `puzzle`, `shmup`, `shaders`. `flappy`
is in the repository only (`cd examples/flappy && npm install && npm start` from a checkout).

## Which one to read

| You want to learn | Read |
|---|---|
| UI menus, settings, music, two languages, a best score | [flappy](#flappy) |
| Sprite animation, LDtk levels, camera, raycasts, one-way platforms | [platformer](#platformer) |
| Two players, auto-aim, area queries, rumble, panned sound | [topdown](#topdown) |
| Game rules as pure functions, undo, saves, plural translations | [puzzle](#puzzle) |
| Collision categories, many moving things, layers, postfx | [shmup](#shmup) |
| Custom GPU shaders, lights, on-screen touch buttons | [shaders](#shaders) |

Each section below shows the core idea with an excerpt from the real files.

## flappy

Title, play, game-over and settings scenes; an RmlUi interface navigable by mouse, keyboard and
gamepad; streamed OGG music; English and Portuguese; a TTF pixel font; the best score in the save.

**Effects are data.** The bird carries its particle emitter in its spec; the play scene bursts
it on every flap (`src/prefabs/bird.ts`):

```ts
emitter: {
  lifetime: [0.3, 0.6], speed: [60, 140], angle: [150, 210], gravity: [0, 30],
  size: [4, 1], color: ["#88ffffff", "#a36bff00"], shape: "circle", max: 64,
},
...(physics && { body: { type: "dynamic", collider: { circle: BIRD.radius } } }),
```

**Saves in two lines** (`src/scenes/over.ts`):

```ts
const best = Math.max(score, ctx.save.get("best", 0));
ctx.save.set("best", best);
```

**Hints per platform, text per language** (`src/scenes/title.ts`):

```ts
ctx.ui.set("title", "hint", ctx.t("title.hintTouch"));
if (ctx.audio.musicPlaying !== "theme") ctx.audio.music("theme", { volume: 0.6, fadeIn: 1 });
```

It also ships a committed Android project (`examples/flappy/android`) and npm scripts for every
Android variant. Guides: [Game UI](ui.html), [Audio](audio.html), [Saves, settings,
i18n](saves.html), [Android](android.html).

## platformer

Sprite sheets and animation, an LDtk level, camera follow with bounds and shake, a raycast ground
check, a one-way ledge, a tweened lift, a parallax layer, dust particles and world text.

**A whole level in one call** (`src/scenes/play.ts`): the IntGrid layer `Walls` becomes merged
static colliders tagged `walls`, and each LDtk entity goes through a factory.

```ts
const level = ctx.spawnLevel("main", "Level_0", { collide: { Walls: "walls" }, entities: { Player: (e) => player(e.pos), Coin: coin } });
const lifter = ctx.spawn(lift());
ctx.tween(lifter, { to: { x: LIFT.toX }, seconds: LIFT.seconds, ease: "sineInOut", yoyo: true, repeat: Infinity });

ctx.camera.bounds([0, 0, level.size[0], level.size[1]]);
ctx.camera.follow(hero, { lerp: CAMERA.lerp, deadzone: CAMERA.deadzone });
ctx.onEnter("player", "coin", (_p, c) => pick(ctx, c));
```

**Jump up through a ledge** (`src/prefabs/platforms.ts`):

```ts
body: { type: "static", collider: { rect: [width, 6], oneWay: true } },
```

**Ground check by raycast:**

```ts
const hit = ctx.raycast([p.pos.x + dx, feet - 1], [p.pos.x + dx, feet + GROUND_RAY.reach], { tags: FLOOR });
```

Guides: [Sprites, camera, LDtk levels](tilemaps.html), [Physics](physics.html),
[Particles and effects](particles.html).

## topdown

Two players on one machine, movement axes, auto-aim, sensor bullets, a bomb that clears an area,
one-shot particle blasts, gamepad rumble and sounds panned to where they happen.

**One input per player** (`src/scenes/play.ts`): `ctx.input.player(n)` has the same `axis` /
`down` / `pressed` as `ctx.input`.

```ts
const input = ctx.input.player(h.data.player as Player);
const move: Vec2 = [input.axis("moveX"), input.axis("moveY")];
h.body.velocity = vec.scale(vec.norm(move), amount * HERO.speed);
h.animation = amount > 0 ? "walk" : "idle";
if (input.down("fire") && h.data.cooldown === 0) {
  ctx.spawn(bullet(at(h), aimFrom(ctx, h)));
  ctx.audio.play("shot", { pan: pan(ctx, h.pos.x), volume: 0.4 });
}
```

**Everything in a circle** (a physics area query):

```ts
for (const e of ctx.queryCircle(at(h), BOMB.radius, { tags: ["enemy"] })) if (e.alive) kill(ctx, e, 1);
ctx.camera.shake(BOMB.shake, BOMB.seconds);
ctx.input.rumble(0.6, 0.2, h.data.player as Player);
```

Guides: [Input](input.html), [Physics](physics.html), [Audio](audio.html).

## puzzle

Sokoban. The rules are pure functions on plain data, tested directly without the engine; the
scene only draws what they return. Undo, level progress and best moves in the save, i18n with
plural forms and `data-t`, reduced motion and high contrast.

**Rules with no engine calls** (`src/rules.ts`):

```ts
/** What moves: JSON-safe, so it lives in ctx.state (kept by hot reload) and undo keeps copies. */
export interface Position {
  player: Cell;
  boxes: Cell[];
}
```

**Tested as plain function calls** (`test/main.test.ts`):

```ts
const { level, start } = parse(["#####", "#@$$#", "#####"]);
check(move(level, start, "right") === null, "two boxes in a row cannot be pushed");
```

**Translated text with counts** (`src/scenes/play.ts`):

```ts
ctx.ui.set("hud", "moves", ctx.t("hud.moves", { count: ctx.state.moves }));
```

Guides: [Saves, settings, i18n](saves.html), [Text and languages](text.html),
[Testing](testing.html).

## shmup

A vertical shooter: collision categories, kinematic enemies, fast sensor shots, 120 stars moved
in bulk, parallax and fx layers, generated music with fade in and out, a blink tween, a CRT
postfx toggle.

**Who collides with whom** (`src/prefabs/actors.ts`):

```ts
body: { type: "kinematic", velocity: [0, fall], category: "enemy", collidesWith: ["ship", "shot"], collider: { rect: [14, 12] } },
```

**Moving many entities at once** (`src/scenes/play.ts`): read every position into arrays, edit
them, write them back once.

```ts
const stars = ctx.positions("star");
for (let i = 0; i < stars.count; i++) {
  stars.y[i] += STARS.speeds[i % 3] * ctx.dt;
  if (stars.y[i] > vy + vh) stars.y[i] -= vh;
}
stars.write();
```

**Getting hit feels like it:**

```ts
ctx.camera.shake(3, 0.3);
ctx.input.rumble(0.8, 0.25);
ctx.tween(s, { to: { alpha: 0.2 }, seconds: SHIP.safeSeconds / 8, yoyo: true, repeat: 3 });
if ((s.data.lives as number) <= 0) ctx.after(0.5, () => ctx.go("over", { score: ctx.state.score }));
```

Guides: [Physics](physics.html), [Particles and effects](particles.html).

## shaders

Six custom Vulkan GLSL fragment shaders (wave, chromatic aberration, sepia, posterize, scanlines,
vignette pulse) switched live, with `intensity` and `time` uniforms, plus lights, emitters and
on-screen buttons on phones.

**Declare a shader with its uniforms** (`src/main.ts`):

```ts
const shader = (name: string) => ({ file: `assets/shaders/${name}.frag`, uniforms: { intensity: INTENSITY.start, time: 0 } });
```

**Switch them at runtime** (`src/scenes/lab.ts`):

```ts
ctx.postfx.remove(SHADERS[ctx.state.index]);
ctx.state.index = cycle(ctx.state.index, step);
ctx.postfx.add(SHADERS[ctx.state.index], { intensity: ctx.state.intensity, time: ctx.time });
```

`alpha run` compiles the `.frag` files with glslang (and recompiles them on save under
`--watch`); `alpha export` packs SPIR-V. Guide: [Shaders and lighting](shaders.html).

## Change one thing and see it

The fastest way to learn from an example: change one value, play, run the tests.

1. In `my-platformer/src/main.ts`, change `background` to `"#1d2b53"` and run `npm start`.
   **Expected:** a dark blue sky; everything else the same.
2. Open `src/tuning.ts` and change one number (a jump height, a speed). Play again.
3. Run `npm run check` and `npm test`. If a test fails after a gameplay change, read what it
   expected before editing the test: it may have caught an accidental change to the rules.

## Running an example from a repository checkout

```
cd examples/shmup
npm install
npm test
npm start
```

Placeholder art is generated by scripts in `tools/` (CC0) or drawn from shapes; bundled fonts have
their own license files. Preview every game in the [showcase](index.html).

## See also

- [Getting started](getting-started.html): the `blank` template, step by step.
- [Concepts](concepts.html): scenes, entities, prefabs and the names in `types.ts`.
- [Testing](testing.html): every example's `test/main.test.ts` is a recipe book.
