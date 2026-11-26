## Pixel-perfect movement and the camera (render interpolation)

The engine simulates on a fixed step and **draws every entity and the camera interpolated**
between the previous and the current step (smooth on 120/144 Hz screens). Follow these rules or
sprites will shake or flicker:

- **Follow the player with `ctx.camera.follow(hero, { lerp: 1 })`.** Never set `ctx.camera.pos`
  by hand every step, and never round it (`Math.round`): the camera then jumps in whole pixels
  while the hero is drawn interpolated, and the hero shakes ~0.5 px in the walking direction.
  Use `lerp < 1` only if you want a trailing camera (the world then seems to slide when it catches up).
- **Put still things (scenery, props, tiles) on whole pixels** (`Math.round` their spawn position).
  With a fractional camera they then all shift by the same fraction and move as one block.
  Mixing fractional and whole positions makes them wobble against each other.
- **Teleports need nothing special.** Setting `pos` more than a quarter of the view away is drawn
  there at once (children too). A shorter jump is drawn sliding over one step, so when you recycle
  or relocate entities, keep recycled slots fixed to a world cell (`cell mod gridSize`): only
  off-screen slots move, and they move a whole grid.
- **Size everything that fills the screen from `ctx.camera.view`** (ground tiles, scenery grids,
  spawn and despawn distances), never from fixed numbers: the visible world changes with the
  window, the monitor and the phone (`config.window.aspect`). Recompute when the view size changes.
- **`config.size` is the design canvas, not the window resolution.** Art, UI (1dp = 1 design px),
  lights and distances are measured in it; the engine scales it to any screen. Raising it to a
  "bigger resolution" makes everything smaller. Pick it once for the art (e.g. 384x216 for 32 px
  sprites) and leave it.
- **Test sizes headless:** `simulate(game, { size: [1080, 2340] })` and check coverage against the
  run's `ctx.camera.view` (store it in `ctx.state` to assert on it).
