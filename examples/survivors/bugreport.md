# Bug report: alpha-star engine (round 2)

Found while polishing Candlewick's UI (`examples/survivors`). The earlier `image(file fit)`
packing bug is already fixed in `alpha-sdk/lib/pack.mjs`.

---

## 0. The first window is always 2x the design size, even when that is bigger than the monitor

**Status: fixed**: `initial_window_size` fits 90% of the display's usable bounds; docs say `size` is the design canvas.

**Severity:** high. Any game with a larger `config.size` opens partly off-screen, title bar
included. Candlewick with `size: [1024, 768]` opened a **2048x1536** window (MCP screenshot size)
on a smaller display, and the game looked broken.

**Cause**: `src/core/window.cpp:59`

```cpp
Vec2 initial_window_size(Vec2 game, Scale scale) { return game * (scale == Scale::Integer ? 2.0f : 1.5f); }
```

`platform/window.cpp:62` passes that straight to `SDL_CreateWindow`. Nothing checks the display.

**Fix**: fit the window to the display's usable area. Keep it pure in `core` and give it the
bounds from `platform`:

```cpp
// core/window.cpp: largest whole multiple (integer) or 90% fit (fractional) that fits `usable`, at most 2x.
Vec2 initial_window_size(Vec2 game, Scale scale, Vec2 usable) {
  const float fit = std::min(usable.x / game.x, usable.y / game.y) * 0.9f;
  const float k = scale == Scale::Integer ? std::max(1.0f, std::min(2.0f, std::floor(fit))) : std::min(1.5f, fit);
  return game * k;
}
// platform/window.cpp: SDL_Rect r; SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &r)
// (fall back to the old 2x / 1.5x when that fails, e.g. headless).
```

Tests in `tests/core/`: `[384,216]` on 1920x1080 gives 768x432; `[1024,768]` on 1920x1080 gives
1024x768 (integer) and on 1280x720 a fractional fit inside the screen; a huge design never gets
smaller than 1x for integer (the user can still resize).

**Related design gap (not a bug, worth a doc line):** `config.size` is the reference resolution
for the world *and* the UI (1dp = 1 design pixel, like Unity's Canvas Scaler). Raising it from
384x216 to 1024x768 also makes every sprite and UI element 2.7x smaller on screen. That is
expected, but agents and users read `size` as "window resolution". Suggestion: say it plainly in
`camera-and-window.md` ("size is the design canvas, not the window"), and optionally add
`config.window.size` for the starting window, separate from the design size.

## 1. UI images (RmlUi) are always drawn with linear filtering: pixel-art UI comes out blurry

**Status: fixed**; `config.uiFilter` now defaults from `config.pixelMode`. (`config.uiFilter` in `src/ui/rml_renderer.cpp`, `ui.cpp`). Candlewick
now sets `uiFilter: "nearest"` and the cards render crisp. Still worth considering: default it to
`config.pixelMode` so pixel-art games get it without knowing the flag.

**Severity:** medium (looks). `config.pixelMode` (default true) and `window.scale: "integer"` keep
the world crisp. Every image a `.rml`/`.rcss` uses (`decorator: image(...)`, `<img>`), though, is
scaled with linear filtering, so pixel-art buttons, frames and logos look smeared at 2x to 4x.

**Cause**: `src/ui/rml_renderer.cpp:96-97`

```cpp
SDL_SetTextureBlendMode(tex->uploaded, premultiplied_);
if (tex->nearest) SDL_SetTextureScaleMode(tex->uploaded, SDL_SCALEMODE_NEAREST);
```

Only generated textures (glyph atlases from `GenerateTexture`) get `nearest`. `LoadTexture`
(image files, line ~101) never sets it, and nothing reads `config.pixelMode` or a per-image
`filter` for UI.

**Fix**: give `RmlRenderer` the game's pixel mode and use it for file textures:

```cpp
// LoadTexture
return reinterpret_cast<Rml::TextureHandle>(new Texture{dimensions, std::move(image.rgba), nullptr, pixel_mode_});
```

Optionally honour an `assets.images` entry's `filter` when the UI file is also declared there.
Docs: add "UI images follow `config.pixelMode`" to `docs/llm/ui.md` and `sprites-and-images.md`.
Test: a UI doc that draws a 2x2 checker PNG at 64x64 dp gives hard edges in a screenshot when
`pixelMode` is true.

**Workaround in this game:** `tools/process-art.py` saves UI images pre-enlarged x4 with
nearest-neighbour, so linear filtering only softens the 1-texel edges.

---

## 2. Moving an entity far by script draws it halfway for one frame (render interpolation)

**Status: fixed** (option 2): setting `pos` farther than 1/4 of the view is drawn there at once; no API.

**Severity:** high (looks). Any teleport flashes: recycled scenery and tiles, enemies relocated
around the hero, anything a game moves by assigning `e.pos`.

**Cause:** the world draws every transform blended from the previous step to the current one
(`ecs/world.cpp:281` `snapshot()` stores `Previous`, `ecs/effects.cpp:23` `blended(...)`). A
script setter (`e.pos.x = ...` -> `World::place`) changes `Transform` and leaves `Previous`
alone. A jump from A to B is then drawn at `lerp(A, B, blend)` on the next rendered frame, in
the middle of the screen.

**Evidence (MCP, Candlewick):** ground and scenery slots recycled off-screen were still seen
flashing across the view, and so were enemies relocated past the edge. With the entity hidden
(`alpha = 0`) during the step it moves, the flash goes away.

**Fix options (pick one):**
1. An explicit teleport: `e.warp([x, y])` (or `e.pos.set(x, y, { snap: true })`) that sets
   `Previous = Transform` for that entity. Smallest, and the clearest for agents.
2. Snap automatically when a script moves an entity farther than a threshold in one step (say
   more than 1/4 of the view). Nothing to learn, but magic.

Either way, document it in `docs/llm/entities.md` ("positions are drawn interpolated; use warp to
jump"). Also check `ctx.camera.pos` assignments: `previous_view_` has the same issue for camera
cuts.

**Workaround in this game:** `warp()` / `unwarp()` in `src/game/world.ts` (hide for the teleport
step, show on the next).

## 3. `float` layout in RmlUi documents (unconfirmed, maybe RmlUi itself)

`float: left` on `<button>`s inside a 300dp panel stacked them one per line instead of two per
row. `clear: both` had no visible effect. Two `display: inline-block` columns worked. This may
be RmlUi behaviour for `button` elements rather than the engine; check it against RmlUi's
samples before changing anything.

---

## 4. No way to know the player's system language

**Severity:** low (missing API). `ctx.locale` starts at the saved choice or `config.locale`. A game
can't open in the player's own language on the first launch: there's no `ctx.platform.languages`.
Candlewick first worked around it with a "first launch" flag in the save. That flag went stale
(a save that once had English stayed English), and the user saw the whole game in English.

**Fix**: read `SDL_GetPreferredLocales()` once at boot. When settings have no locale, pick the
first preferred language that is in `assets.locales` (match "pt-BR" to "pt"), else `config.locale`.
Optionally expose the list as `ctx.platform.languages` (read-only). Document it in
`saves-and-i18n.md`; test it with `simulate(game, { languages: ["pt-BR"] })`.

## Checked and not engine bugs

- **Still sprites wobbling / hero shaking while walking.** These were game-side. Props at fractional
  positions rounded differently from the ground tiles. Rounding the camera by hand (an earlier fix)
  then made the interpolated hero shake half a pixel. The right setup is
  `camera.follow(hero, { lerp: 1 })`, with scenery placed on whole pixels. Worth a docs tip in
  `camera-and-window.md` for pixel-art games.
