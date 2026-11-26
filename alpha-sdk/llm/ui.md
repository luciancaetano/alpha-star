# UI (RmlUi)

> summary: Game UI with RmlUi .rml/.rcss documents: declaring docs and element ids, show/hide/set/toggle/attr/on (text, classes, images, bars), full-screen layout and class="safe", focus and gamepad navigation, pixel-art UI images (config.uiFilter), accessibility hooks and UI tests; read when building HUDs, menus or buttons.

## API

```ts
// types.ts: document -> ids that scripts touch (`never` = show/hide only, `{}` = no UI)
ui: { hud: "level" | "moves" | "best"; solved: "moves" | "next" };
// main.ts assets: files they link (<link href>, src=, rcss url()/image(file fit...), @spritesheet src:) are packed too
ui: { hud: "assets/ui/hud.rml", solved: "assets/ui/solved.rml" },

ctx.ui.show(doc); ctx.ui.hide(doc);
ctx.ui.set(doc, id, text | number);           // plain text, never parsed as markup
ctx.ui.toggle(doc, id, className, on);       // one class on/off (style states in .rcss); `on` required
ctx.ui.attr(doc, id, name, value);           // text | number | true (present) | false / null (removed):
                                             // <img src> (relative to the .rml), <progress value> 0..1, disabled
ctx.ui.on(doc, id, "click" | "back", fn);     // handlers run at the start of the next step
```

## Example (examples/puzzle)

```xml
<rml>
<head><link type="text/rcss" href="crates.rcss"/></head>
<body>
  <div class="panel">
    <h1 data-t="solved.title">SOLVED!</h1>
    <p id="moves">IN 0 MOVES</p>
    <button id="next" autofocus data-t="solved.next">NEXT LEVEL</button>
  </div>
</body>
</rml>
```

```ts
// scenes/solved.ts enter (moves, next computed from params)
  ctx.ui.set("solved", "moves", ctx.t("solved.moves", { count: moves }));
  ctx.ui.on("solved", "next", "click", () => ctx.go("play", { level: next }));
  ctx.ui.show("solved");
```

## Rules and gotchas

- Documents load hidden before the first scene. On every scene change all documents reload hidden (texts back to the file's) and handlers are dropped: show/set/on again in `enter`.
- Focusable elements (buttons) need `tab-index: auto` in the RCSS; style `:hover`, `:focus`, `:active`. `autofocus` focuses a button.
- Gamepad: D-pad / left stick move focus, A clicks, B is "back". Mouse and touch click. Escape / B fire `"back"` while the doc is shown.
- Input the UI consumes never also triggers game actions.
- Drawn over the game in screen (game) pixels across the whole revealed screen (config.window.aspect: phones and resized windows show more than config.size), so `right: 0` / `bottom: 0` are the real edges; it scales with the kept axis like Unity's Canvas Scaler. Camera and postfx never affect it.
- `class="safe"` on an element (e.g. `<body class="safe">`, with `box-sizing: border-box` when it is `width/height: 100%`) pads it by `ctx.safeArea`, so HUDs stay clear of cutouts and rounded corners.
- `data-t="key"` elements show `ctx.t(key)` and re-translate when `ctx.locale` changes (saves-and-i18n.md).
- `ctx.settings.uiScale` (0.5..3) is the RmlUi dp ratio: size things in `dp`. `ctx.settings.highContrast` adds class `high-contrast` to every body: style `.high-contrast ...`.
- Fonts: `font-family` = an `assets.fonts` name (its `fallbacks` apply); none declared = built-in 8x8 font (multiples of 8). `direction: rtl` for right-to-left paragraphs, `word-break: break-all` to wrap CJK; `font-effect` is not drawn (text-and-fonts.md).
- UI images (`<img>`, `decorator: image(...)`) are sampled by `config.uiFilter`: `"nearest"` (pixel-art UI; default when `config.pixelMode` is true, the default), `"linear"` (smooth; default with `pixelMode: false`), `"sharp"` (crisp pixels, soft edges at any scale). UI text is always made at the output resolution.
- Use RmlUi for game UI; Dear ImGui (F12) is engine dev tools only and absent from shipped games.
- Errors in ui calls name the call: `ui.set("hud", "x", ...): ...`.
- Tests: `simulate(game, { seconds: 3, clicks: { "solved#next": [120] } })`; `run.expect.text("hud", "moves").equals("3 MOVES")`, `run.expect.shown("solved")`. Clicking a hidden doc or unknown id fails the test.
