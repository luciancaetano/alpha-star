# Concepts

Start with [Getting started](getting-started.md) and run the `blank` template before reading this
page. You only need a few ideas to understand that game's code.

| Word | Meaning | Example in the blank template |
|---|---|---|
| Game | The settings and scenes that make up your project | `src/main.ts` |
| Scene | One part of a game, such as a menu or a level | `src/scenes/play.ts` |
| Entity | One object in the scene, such as the player | The object created by `ctx.spawn(...)` |
| Asset | A file or sound your game uses | `assets/img/player.png` |
| Context (`ctx`) | The tools the engine gives your scene | `ctx.input`, `ctx.spawn`, `ctx.audio` |
| Prefab | A function that describes an object you can create | `src/prefabs/player.ts` |

The first five sections are all you need for the beginner guides. The last three
(**Pack**, **Build flavors**, **Image filtering and window stretch**) are for later: come back when
you share a game or tune how it looks on different screens.

## Game, scenes, ctx

Open `src/main.ts` in the blank project. `game(...)` connects four things:

- `config` sets the window title, game size and controls.
- `start: "play"` chooses the first scene.
- `scenes: { play }` makes the imported play scene available to the game.
- `assets` gives names to images and sounds so your scenes can use them.

Now open `src/scenes/play.ts`. The engine calls the functions inside `scene("play", {...})`
for you. You do not need to write a loop yourself.

| Function | When it runs | What belongs here |
|---|---|---|
| `state(params)` | When entering the scene | Fresh data, such as a score starting at zero |
| `enter(ctx)` | Once when the scene starts | Create the player and level |
| `update(ctx)` | Every simulation step (1/60 of a second) | Read controls and move objects |
| `exit(ctx)` | When leaving the scene | Work needed before the next scene |

Each function is optional. The blank template only needs `enter` and `update`.

For example, this is the `enter` function from the blank template. It is **part of**
`src/scenes/play.ts`, not a complete file; `player` and `VIEW` are imported at the top of that file.

```ts
enter(ctx) {
  // player(...) describes an object; ctx.spawn(...) creates it in this scene.
  // Half the width and half the height place it at the center of the game.
  ctx.spawn(player([VIEW.width / 2, VIEW.height / 2]));
},
```

`ctx` is short for context. Use it to create objects, read input and play sounds.
Its tools belong to the current scene: entities, handlers, tweens (animated changes) and timers
are cleaned up when you change scenes. In a game that declares an `over` scene accepting a score,
`ctx.go("over", { score })` requests that scene at the end of the current step.

## Entities

An entity is an object your game can draw or move. Its **spec** is a description of what to
create. The blank template's `player(at)` function returns this description in
`src/prefabs/player.ts`:

```ts
// Inside player(at): `at` is a position such as [160, 120].
return {
  tags: ["player"],             // A label used to find this object later.
  pos: at,                      // [x, y]: x grows right; y grows down.
  shape: { image: "player" },   // The image named in src/main.ts -> assets.images.
};
```

Creating this description does not put anything on screen: `ctx.spawn(...)` does that.
A prefab is simply a function returning a spec, so you can reuse it at different positions.

In `update(ctx)`, the blank template uses `const p = ctx.find("player")` to get the player it
created in `enter`. Changing `p.pos.x` moves that player horizontally. It multiplies speed
by `ctx.dt` (seconds per simulation step): 120 pixels per second means 2 pixels per 1/60-second step.

**Try it:** change `PLAYER.speed` in `src/tuning.ts`, save, then run `npm start` again.
Hold an arrow key. The player should move at the new speed.

Later, you can add these fields to a spec as needed:

| Field | What it adds | Read next |
|---|---|---|
| `shape` | A circle, rectangle, image, animated sprite or world text | [Sprites and tilemaps](tilemaps.md) |
| `body` | Physics and collisions | [Physics](physics.md) |
| `emitter` | Particles such as sparks or dust | [Particles](particles.md) |
| `data` | Your own JSON game data on an entity | [API reference](reference.md) |

Use `ctx.findAll` when several entities share a tag. `ctx.destroy(e)` schedules removal at the
end of the step; `e.alive` becomes false immediately.

## Determinism

Determinism means you can repeat a game simulation with the same starting values and inputs.
This helps you test a rule, such as "holding Right moves the player right", without playing by hand.

The simulation uses a fixed time step. For random choices, use `ctx.random`; `config.seed`
sets its starting point. `Math.random` is disabled. Particles, tweens, the camera and animations
also advance on the fixed step; audio is output only. See [Testing](testing.md) for
`simulate(game, { seconds, inputs })` examples.

## Errors

A misspelled name often shows up before the game starts. Run `npx alpha check` from your game
folder, read the first error, and open the file and line it names. The names allowed by your
game live in `src/types.ts`; they must also match the configuration in `src/main.ts`.

A script error during play pauses the game and shows a message pointing back to your source.
It tells you what failed and, when available, the valid choices. For example, an unknown animation
error lists the animations in that sheet. Fix the error, save and run again.

---

**Later topics.** You can stop here and continue with [Examples](examples.md).

## Pack

A **pack** is the folder of game code and assets the engine runs. You normally do not edit it:
edit `src/` and `assets/`, then let the SDK generate it.

`npx alpha export` checks your TypeScript, combines `src/main.ts` and its imports into
`dist/main.js`, and copies the declared assets. The JavaScript includes an inline source map,
which lets errors point back to your TypeScript. PNG images and sprite sheets are combined into
atlas pages (larger images containing several smaller ones). Font licenses and engine notices
go into `THIRD_PARTY_NOTICES.txt`.

To share a game with someone who has no SDK installed, follow [Exporting](exporting.md).

## Build flavors

These are versions of the engine for different jobs. The CLI selects one for you; you do not
need to build the engine to follow the beginner guide.

| Flavor | Has | Used by |
|---|---|---|
| dev | everything, debug build | engine development |
| release | dev tools (F12; phone: shake / Menu key), MCP control channel, headless flags, test runner | `alpha run`, `alpha test`, `alpha mcp` |
| shipping | none of those; LTO, stripped, no console | `alpha export --standalone` (players) |

Nothing in a game can depend on dev features: they are absent from the shipping engine.

## Image filtering and window stretch

Return here when you want to adjust how your game looks at different window sizes.
**Filtering** controls how an image looks when it gets larger, smaller or rotated.

Each image, sheet or map picks how it is sampled when drawn scaled or rotated:

```js
assets: { images: {
  hero:  { file: "img/hero.png", filter: "sharp" },   // pixel art at any size/angle: square pixels, soft edges
  tiles: { file: "img/tiles.png", filter: "nearest" },// crisp; best at whole-number scales, jagged when rotated
  sky:   { file: "img/sky.png", filter: "linear" },   // smooth art and photos
} }
```

Without `filter`, `config.pixelMode` decides (`true`: nearest, `false`: linear). `"sharp"` costs
16x the texture memory of that image.

`config.window` sets how the game's `size` (the design resolution) fits the screen and how the
window opens. The model is Godot's stretch aspect + scale mode:

```js
window: {
  aspect: "expand",     // expand (default) | keep_width | keep_height | keep
  scale: "fractional",  // fractional (default) | integer
  borderless: false, alwaysOnTop: false, maximized: false,
  vsync: true, resizable: true, fullscreen: false, altEnter: true,
}
```

| aspect | screen wider than the design | screen taller than the design |
|---|---|---|
| `expand` (default) | more world left and right | more world above and below |
| `keep_width` | side bars | more world above and below |
| `keep_height` | more world left and right | bars above and below |
| `keep` | side bars | bars above and below |

The revealed world splits evenly around the design center. `ctx.camera.view` is the world rect
the screen shows (`[x, y, w, h]`): size backgrounds, ground and walls to it and spawn at its
edges; keep difficulty in design numbers. `scale: "integer"` keeps pixel art exact (whole
multiples); the revealed axes still fill the screen up to under one scale step.

Screen space (layers with `parallax: 0`, `ctx.safeArea`, the game UI) covers the whole revealed
screen, so a HUD anchored to an edge sits at the real edge. UI documents scale with the kept axis,
like Unity's Canvas Scaler matching width or height; `class="safe"` pads an element by the safe
area. `config.window.scaling` was replaced (`letterbox` -> `aspect: "keep"`, `integer` ->
`scale: "integer"`); the error names the replacement. Tests: `simulate(game, { size: [1080, 2340] })`.
