## Lessons from building a full game (Candlewick)

### Entities and queries
- **Queries and `findAll` return only living entities**, but a list you hold goes stale while
  your own code kills: if one strike can kill the next target, check `t.alive` again on every
  iteration (`e.pos` throws on a dead entity).
- **Keep entity data in the tag's type** (`tags: { enemy: { hp: number } }`): `e.data.hp` is
  then a number everywhere, spawning without it is a compile error, and no cast is ever needed.
- **Entities without a body are invisible to queries.** Orbiting lanterns, pools and slashes with
  no `body` are not found by `queryCircle`; use `findAll(tag)`.
- **Several contacts arrive in one step.** Guard `onEnter`/`onHit` handlers with `alive`
  (or a `data` flag) before acting.

### Pausing the world (menus, level-up)
- **`ctx.frozen = true` while a menu is up**: physics (velocities kept), contacts, `ctx.every`/
  `ctx.after`, tweens, animations, particles and `ctx.time` stop; `update`, UI handlers, input and
  audio keep running. Skip your own world systems in `update` while frozen (they would still run).

### UI (RmlUi)
- **`ctx.ui.set` is text only.** To show item icons in the UI, make a BMFont whose glyphs are the
  icons (`A` = sword, `B` = bow...) and `set` those letters; bars work the same way with repeated
  glyphs (`"####----"`).
- **Size the UI in `dp` = design pixels**, for the `config.size` canvas (a 384x216 game has
  384dp of width). Plain RCSS panels (`background-color` + `border`) are always crisp; avoid
  stretching one big frame image across the screen.
- **Pixel-art UI images** are crisp by default (UI images follow `config.pixelMode`; override with
  `config.uiFilter`). Draw UI art at its real pixel size and give elements that same size (or a
  whole multiple).
- `float` layouts were unreliable; use `display: inline-block` columns for grids.
- Whitespace collapses: separate inline items with margins, not spaces.
- Two `game_click`s in the same step may conflict: step at least one frame between clicks.

### Languages
- **Every player-facing word lives in `assets/locales/*.json`**: `ctx.t(key)` in code, `data-t`
  in RML. Keep the same keys in every file (a quick test that compares the key sets catches gaps).
- **Default language = `config.locale`.** Don't invent "first launch" flags in the save to pick
  a language: they go stale and lock the player into the wrong language.
- Pure rule functions (offers, chests) take a `t` function, so tests can pass `(k) => k` and
  assert on keys.

### Balance and pacing
- **Spawn rate, not batches:** accumulate `rate * dt` and spawn one foe per whole unit; blend
  rate and cap from one minute to the next. Batches every N seconds read as "all at once".
- **Contact damage must add up per touching foe**, with a short invulnerability (~0.3 s). One hit
  at a time from the strongest foe makes standing still immortal.
- Write a test that a hero who never moves dies, and one that pacing never jumps
  (`waveAt(t)` vs `waveAt(t - 1)`).
- Relocate foes left far behind **well past the view corners** (half the view diagonal + margin),
  or they pop in right next to the player.

### Art pipeline (generated images)
- Generate sprites on a flat magenta background; key it out and slice frames at the widest gaps
  (the expected count is known), then downscale with a premultiplied box filter and hard alpha.
- One fixed cell size per sheet; scale every frame of a sheet by the same factor.
- Keep the raw images in the repo and the processing as a script (`tools/process-art.py`):
  never hand-edit the generated sheets.

### Working with the MCP
- Play-test with a scripted bot: `game_input` axis + `press` + `game_step`, `game_screenshot`
  at key moments. Long runs find real crashes (the stale-target bug was found this way).
- To debug movement, step 1 frame at a time with `game_state` and diff positions between frames:
  it separates simulation bugs (positions jump) from drawing bugs (positions fine, screen wrong).
- Put debug-relevant values in `ctx.state` or entity `data`: `game_state` and tests can read them.
