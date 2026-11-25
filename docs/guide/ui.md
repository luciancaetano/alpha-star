# Game UI

Game UI is [RmlUi](https://mikke89.github.io/RmlUiDoc/): HTML/CSS-like `.rml` documents with
`.rcss` styles, declared in `assets.ui` and loaded hidden before the first scene.

```ts
// types.ts: document -> ids scripts touch
ui: { hud: "score"; over: "score" | "again" };
// main.ts
assets: { ui: { hud: "assets/ui/hud.rml", over: "assets/ui/over.rml" } }
```

```xml
<rml>
<head><link type="text/rcss" href="game.rcss"/></head>
<body>
  <p id="score">0</p>
  <button id="again" autofocus>AGAIN</button>
</body>
</rml>
```

```ts
ctx.ui.show("hud");
ctx.ui.set("hud", "score", ctx.state.score);   // plain text, never parsed as markup
ctx.ui.on("over", "again", "click", () => ctx.go("play"));
ctx.ui.on("settings", "back", "back", () => ctx.go("title"));   // Escape / gamepad B
```

- Buttons need `tab-index: auto` in the RCSS to be focusable; style `:hover`, `:focus`, `:active`.
- Gamepad: D-pad / left stick move focus, A clicks, B is "back". Mouse and touch click.
- On every scene change the documents reload hidden and handlers are dropped.
- Sizes in `dp` follow `ctx.settings.uiScale`; `.high-contrast` is set on every body when
  `ctx.settings.highContrast` is on.
- Text is rasterized at the screen's real resolution: sharp at any window size.

## Fonts

`assets.fonts: { pixel: { file: "assets/fonts/PressStart2P-Regular.ttf", license: "assets/fonts/OFL.txt" } }`;
the name is the `font-family` in RCSS and `font` of world text. TTF/OTF or BMFont `.fnt`. With no
fonts at all, every family falls back to the built-in 8x8 font (public domain): use sizes in
multiples of 8 (`examples/topdown`, `puzzle`, `shmup` do).

## In tests

```ts
simulate(game, { seconds: 3, clicks: { "over#again": [120] } }).expect.scene("play");
run.expect.text("hud", "score").equals("3");
run.expect.shown("over");
```
