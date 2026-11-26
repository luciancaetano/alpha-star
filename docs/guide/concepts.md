# Concepts

## Pack

`alpha export` type-checks `src/main.ts`, bundles it into one ES module (`dist/main.js`, with an
inline source map) and copies exactly the declared assets next to it. That folder is the **pack**:
the engine runs it. PNG images and sprite sheets are packed into atlas pages; fonts' license files
go into `THIRD_PARTY_NOTICES.txt` with the engine's own notices.

## Game, scenes, ctx

```ts
export default game({
  config: { title: "Arena", size: [320, 240], seed: 7, actions: { fire: ["Space", "GamepadA"] } },
  start: "play",
  scenes: { play, over },
  assets: { sounds: { shot: { wave: "square", freq: [900, 500], duration: 0.05 } } },
});
```

A scene is `{ state?, enter?, update?, exit? }`. `state(params)` builds fresh state on every enter;
`update(ctx)` runs once per fixed step (1/60 s). `ctx` is everything a scene can touch: `spawn`,
`find`, `input`, `audio`, `ui`, `camera`, `save`, `random`, `tween`, `every`, `after`, `go`...
It is scene-scoped: entities, handlers, tweens and timers go away on scene change.
`ctx.go("over", { score })` switches at the end of the step; params are typed per scene.

## Entities

An entity is spawned from a spec: plain data built by functions (prefabs), never classes.

```ts
ctx.spawn({ tags: ["coin"], pos: [x, y], shape: { sheet: "coin", animation: "spin" },
            body: { type: "static", collider: { rect: [8, 8], sensor: true } }, data: { value: 1 } });
```

`tags` are how scenes find entities (`ctx.find`, `ctx.findAll`, collision handlers). `shape` is
a circle, rect, image, sheet sprite or world text; `body` makes it physical; `emitter` makes
particles; `data` is JSON game data on the entity. `ctx.destroy(e)` removes it at the end of the
step and `e.alive` is false at once.

## Determinism

The simulation runs on a fixed step with a random seeded by `config.seed` (`ctx.random`;
`Math.random` is disabled). Particles, tweens, the camera and animations all advance on the fixed
step. Audio is output only. So `simulate(game, { seconds, inputs })` gives the same result every
time, on every machine: agent tests are exact, never flaky.

## Errors

A script error pauses the game and shows a source-mapped message; it never crashes the engine.
Messages are written for the agent that will fix them: what failed and what exists (e.g. an
unknown animation lists the sheet's animations).

## Build flavors

| Flavor | Has | Used by |
|---|---|---|
| dev | everything, debug build | engine development |
| release | dev tools (F12; phone: shake / Menu key), MCP control channel, headless flags, test runner | `alpha run`, `alpha test`, `alpha mcp` |
| shipping | none of those; LTO, stripped, no console | `alpha export --standalone` (players) |

Nothing in a game can depend on dev features: they are absent from the shipping engine.

## Image filtering and window stretch

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
