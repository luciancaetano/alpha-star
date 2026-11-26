# Game UI

**What you'll build:** a tap-for-points game with a score/time HUD and a pause menu (Resume,
Restart) that works with mouse, touch, keyboard and gamepad.

Game UI is [RmlUi](https://mikke89.github.io/RmlUiDoc/): HTML/CSS-like `.rml` documents styled by
`.rcss`, declared in `assets.ui`, loaded hidden before the first scene and drawn over the game.
Scripts only do four things with it: `show`, `hide`, `set` text, and `on` click/back.

## Complete example

`src/main.ts`:

```ts
import { define } from "alpha";

interface Clicker {
  actions: "tap" | "pause";
  sounds: "pop";
  images: never;
  tags: "target";
  scenes: { play: undefined };
  ui: {
    hud: "score" | "time";             // document -> ids of the elements scripts touch
    pause: "resume" | "restart";
  };
}

const { scene, game } = define<Clicker>();

const play = scene("play", {
  state: () => ({ score: 0, left: 30, paused: false }),
  enter(ctx) {
    ctx.spawn({ tags: ["target"], pos: [160, 90], shape: { circle: 16, color: "#ffd166" } });
    ctx.ui.show("hud");
    ctx.ui.set("hud", "score", 0);

    const resume = () => { ctx.state.paused = false; ctx.ui.hide("pause"); };
    ctx.ui.on("pause", "resume", "click", resume);
    ctx.ui.on("pause", "resume", "back", resume);           // Escape / gamepad B
    ctx.ui.on("pause", "restart", "click", () => ctx.go("play"));
  },
  update(ctx) {
    if (ctx.input.pressed("pause") && !ctx.state.paused) {
      ctx.state.paused = true;
      ctx.ui.show("pause");
    }
    if (ctx.state.paused) return;

    ctx.state.left = Math.max(0, ctx.state.left - ctx.dt);
    ctx.ui.set("hud", "time", Math.ceil(ctx.state.left));
    if (ctx.input.pressed("tap") && ctx.state.left > 0) {
      ctx.state.score += 1;
      ctx.ui.set("hud", "score", ctx.state.score);
      ctx.audio.play("pop");
    }
  },
});

export default game({
  config: {
    title: "Clicker",
    size: [320, 180],
    background: "#1d2b53",
    seed: 1,
    actions: { tap: ["Space", "MouseLeft", "Touch", "GamepadA"], pause: ["Escape", "P", "GamepadStart", "Back"] },
  },
  start: "play",
  scenes: { play },
  assets: {
    sounds: { pop: { wave: "square", freq: [440, 880], duration: 0.06, volume: 0.2 } },
    ui: { hud: "assets/ui/hud.rml", pause: "assets/ui/pause.rml" },
  },
});
```

`assets/ui/hud.rml`:

```xml
<rml>
<head><link type="text/rcss" href="game.rcss"/></head>
<body class="safe">
  <p class="bar">SCORE <span id="score">0</span>   TIME <span id="time">30</span></p>
</body>
</rml>
```

`assets/ui/pause.rml`:

```xml
<rml>
<head><link type="text/rcss" href="game.rcss"/></head>
<body class="safe">
  <div class="panel">
    <h1>PAUSED</h1>
    <button id="resume" autofocus>RESUME</button>
    <button id="restart">RESTART</button>
  </div>
</body>
</rml>
```

`assets/ui/game.rcss`:

```css
body { width: 100%; height: 100%; box-sizing: border-box; font-size: 8dp; color: #ffffff; }
p { display: block; }
.bar { margin: 4dp 8dp; }
.panel { display: block; width: 140dp; margin: 40dp auto 0dp auto; padding: 10dp; background-color: #000000cc; text-align: center; }
h1 { display: block; font-size: 16dp; margin-bottom: 8dp; }
button { display: block; margin-top: 6dp; padding: 4dp; background-color: #29366f; tab-index: auto; }
button:hover, button:focus { background-color: #ef7d57; }
.high-contrast .panel { background-color: #000000; border: 1dp #ffffff; }
```

`test/main.test.ts`:

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

export function tapsScore() {
  const run = simulate(game, { seconds: 1, inputs: { tap: [0, 10, 20] } });
  run.expect.noErrors();
  run.expect.text("hud", "score").equals("3");
}

export function pauseFreezesTime() {
  const run = simulate(game, { seconds: 2, inputs: { pause: [0] } });
  run.expect.shown("pause");
  run.expect.text("hud", "time").equals("30");
}

export function resumeClosesTheMenu() {
  const run = simulate(game, { seconds: 1, inputs: { pause: [5] }, clicks: { "pause#resume": [30] } });
  run.expect.noErrors();
}
```

No fonts are declared, so the built-in 8x8 font draws everything: keep font sizes at multiples
of 8 (`8dp`, `16dp`). `npx alpha run` to play, `npx alpha test` to test.

## Step by step

### 1. Declare documents and their ids

```ts
// GameTypes: every document, and the ids of elements scripts touch
ui: {
  hud: "score" | "lives";
  title: "play" | "settings";
  loading: never;          // a document scripts only show/hide
};
// game({ assets })
ui: { hud: "assets/ui/hud.rml", title: "assets/ui/title.rml", loading: "assets/ui/loading.rml" },
```

A game without UI writes `ui: {}`. A misspelled document or id is a compile error. Files the
`.rml` links (`<link href>`, `src=`, RCSS `url()`) are packed by `alpha export` too.

### 2. Show, hide, set text

```ts
ctx.ui.show("hud");
ctx.ui.set("hud", "score", 120);     // numbers are fine
ctx.ui.set("hud", "lives", "x3");    // plain text, never parsed as markup
ctx.ui.hide("hud");
```

Result: the element's content becomes exactly that text. Several documents can be shown at once
(a HUD under a pause panel).

### 3. Buttons

```xml
<button id="play" autofocus>PLAY</button>
<button id="settings">SETTINGS</button>
```

```css
button { tab-index: auto; }                         /* required: makes it focusable */
button:hover, button:focus { background-color: #ef7d57; }
button:active { background-color: #16394c; }
```

```ts
ctx.ui.on("title", "play", "click", () => ctx.go("play", { level: 1 }));
ctx.ui.on("title", "settings", "click", () => ctx.go("settings"));
```

`"click"` fires for a mouse click, a touch, or Enter / Space / gamepad A on the focused element.
D-pad and left stick move focus; `autofocus` picks the first focused button. Handlers run at the
start of the next step.

### 4. Back (Escape, gamepad B, Android back)

```ts
ctx.ui.on("settings", "back", "back", () => ctx.go("title"));   // any id of the shown document
```

`"back"` fires while that document is shown. Input the UI uses never also triggers game actions,
so the same Escape does not open your pause menu again.

### 5. Layout: full screen, safe area, dp

```css
body { width: 100%; height: 100%; box-sizing: border-box; }   /* with class="safe" on <body> */
.corner { position: absolute; top: 4dp; right: 4dp; }
```

- Documents cover the whole screen, including what `config.window.aspect` reveals beyond
  `config.size`, so edge-anchored elements sit at the real edges.
- `class="safe"` pads an element by `ctx.safeArea` (phone cutouts, rounded corners). Put it on
  `<body>` and use `box-sizing: border-box`.
- Use `dp` units: they follow `ctx.settings.uiScale`.
- Text is rasterized at the screen's real resolution: sharp at any window size.

### 6. Fonts

```ts
assets: { fonts: { pixel: { file: "assets/fonts/PressStart2P-Regular.ttf", pixelMode: true, license: "assets/fonts/OFL.txt" } } },
```

```css
body { font-family: pixel; font-size: 8dp; }
```

The `assets.fonts` name is the `font-family`. TTF/OTF or BMFont `.fnt`. Without any fonts every
family falls back to the built-in 8x8 font. Other scripts, right-to-left, CJK wrapping:
[Text and languages](text.html).

### 7. Translated text

```xml
<h1 data-t="menu.title">MENU</h1>
```

Elements with `data-t="key"` show `ctx.t(key)` and re-translate when `ctx.locale` changes. See
[Saves, settings, i18n](saves.html).

## Recipes

### Score HUD with lives

```ts
// types: ui: { hud: "score" | "lives" }
enter(ctx) {
  ctx.ui.show("hud");
  ctx.ui.set("hud", "score", 0);
  ctx.ui.set("hud", "lives", "x3");
},
// whenever it changes, not every frame:
function addScore(ctx: Ctx, n: number) {
  ctx.state.score += n;
  ctx.ui.set("hud", "score", ctx.state.score);
}
```

### Title menu

```xml
<body class="safe">
  <div class="panel">
    <h1>MY GAME</h1>
    <button id="play" autofocus>PLAY</button>
    <button id="settings">SETTINGS</button>
  </div>
</body>
```

```ts
export default scene("title", {
  enter(ctx) {
    ctx.ui.show("title");
    ctx.ui.on("title", "play", "click", () => ctx.go("play", { level: 1 }));
    ctx.ui.on("title", "settings", "click", () => ctx.go("settings"));
  },
});
```

### Game over with "play again"

```ts
export default scene("over", {
  enter(ctx, { score }) {
    ctx.ui.set("over", "score", score);
    ctx.ui.show("over");
    ctx.ui.on("over", "again", "click", () => ctx.go("play"));
  },
});
```

(`examples/flappy/src/scenes/over.ts`, which also saves the best score.)

### Settings screen with volume "sliders"

Scripts cannot read form controls, so volumes are `-` / `+` buttons around a value:

```xml
<p>MUSIC <button id="music-down" autofocus>-</button><span id="music">100%</span><button id="music-up">+</button></p>
<p>EFFECTS <button id="sfx-down">-</button><span id="sfx">100%</span><button id="sfx-up">+</button></p>
<button id="contrast">HIGH CONTRAST: <span id="contrast-value">OFF</span></button>
<button id="back">BACK</button>
```

```ts
type Bus = "music" | "sfx";
function nudge(ctx: SettingsCtx, bus: Bus, by: number) {
  const v = Math.round((ctx.audio.bus(bus).volume + by) * 10) / 10;   // no drift
  ctx.audio.bus(bus).volume = Math.min(1, Math.max(0, v));             // persisted
  ctx.ui.set("settings", bus, `${Math.round(ctx.audio.bus(bus).volume * 100)}%`);
}

export default scene("settings", {
  enter(ctx) {
    ctx.ui.show("settings");
    for (const bus of ["music", "sfx"] as const) {
      nudge(ctx, bus, 0);                                     // show the current value
      ctx.ui.on("settings", `${bus}-down`, "click", () => nudge(ctx, bus, -0.1));
      ctx.ui.on("settings", `${bus}-up`, "click", () => nudge(ctx, bus, 0.1));
    }
    ctx.ui.on("settings", "contrast", "click", () => {
      ctx.settings.highContrast = !ctx.settings.highContrast;
      ctx.ui.set("settings", "contrast-value", ctx.settings.highContrast ? "ON" : "OFF");
    });
    ctx.ui.on("settings", "back", "click", () => ctx.go("title"));
    ctx.ui.on("settings", "back", "back", () => ctx.go("title"));
  },
});
```

Full version: `examples/flappy/src/scenes/settings.ts` and `assets/ui/settings.rml`.

### High contrast styles

`ctx.settings.highContrast = true` adds class `high-contrast` to every document's body:

```css
.high-contrast .panel { background-color: #000000; border-color: #ffffff; }
.high-contrast button:focus { background-color: #ffff00; color: #000000; }
```

### Touch hint vs keyboard hint

```ts
ctx.ui.set("title", "hint", ctx.platform.mobile ? "TAP TO START" : "PRESS SPACE");
```

### HUD drawn in the world instead

For a number that follows an entity (a health bar over an enemy), use world text, not UI:
[Text and languages](text.html).

## Gotchas

- Buttons without `tab-index: auto` in the RCSS never get focus (no keyboard or gamepad).
- Every scene change reloads all documents hidden, with the file's texts, and drops all
  handlers: `show` and `on` again in `enter`.
- `ui.set` replaces the element's whole content: put the changing part in its own `<span id>`.
- Only text and visibility of whole documents are scriptable: no reading inputs or sliders, no
  changing classes or styles from scripts. Use a second document for a panel that comes and goes.
- `font-effect` (shadow/outline) is not drawn.
- The UI keeps running while your game "pauses" itself: pausing is your own state flag.
- In tests, clicking a hidden document or an unknown id fails the test.

## See also

- [Text and languages](text.html): fonts, RTL, CJK in UI and world text.
- [Saves, settings, i18n](saves.html): `data-t`, `ctx.settings`.
- [Audio](audio.html): volume buses behind the settings screen.
- `examples/flappy` (title, HUD, game over, settings), `examples/puzzle` (localized HUD),
  `examples/shaders` (touch buttons).
- In `alpha.d.ts`: `UI`, `Settings`, `Context.safeArea`; tests: `SimulateOptions.clicks`,
  `Expect.text`, `Expect.shown`.
