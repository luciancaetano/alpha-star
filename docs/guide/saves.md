# Saves, settings, i18n

**What you'll build:** a score game that keeps the best score across restarts, shows a localized
HUD and game-over panel (English and Portuguese, with plural forms), and switches language with L.

Three stores, each with one job:

| Store | API | File |
|---|---|---|
| Game progress | `ctx.save` (+ named slots) | one file per slot |
| Player preferences | `ctx.settings`, bus volumes, `ctx.locale`, fullscreen, rebinds | `settings.json` |
| Texts per language | `ctx.t`, `data-t` in UI | `assets.locales` JSON (read-only) |

## Complete example

`src/main.ts`:

```ts
import { define } from "alpha";

interface Dodge {
  actions: "point" | "die" | "language";
  sounds: never;
  images: never;
  tags: "player";
  locales: "en" | "pt";
  scenes: { play: undefined; over: { score: number } };
  ui: { hud: "score" | "best"; over: "result" | "again" };
}

const { scene, game } = define<Dodge>();

const play = scene("play", {
  state: () => ({ score: 0 }),
  enter(ctx) {
    ctx.ui.show("hud");
    ctx.ui.set("hud", "score", ctx.t("hud.score", { n: 0 }));
    ctx.ui.set("hud", "best", ctx.t("hud.best", { n: ctx.save.get("best", 0) }));
  },
  update(ctx) {
    if (ctx.input.pressed("language")) ctx.locale = ctx.locale === "en" ? "pt" : "en";
    if (ctx.input.pressed("point")) {
      ctx.state.score += 1;
      ctx.ui.set("hud", "score", ctx.t("hud.score", { n: ctx.state.score }));
    }
    if (ctx.input.pressed("die")) ctx.go("over", { score: ctx.state.score });
  },
});

const over = scene("over", {
  enter(ctx, { score }) {
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);                                  // written to disk now
    ctx.ui.set("over", "result", ctx.t("over.result", { count: score }));
    ctx.ui.show("over");
    ctx.ui.on("over", "again", "click", () => ctx.go("play"));
  },
});

export default game({
  config: {
    title: "Dodge", size: [320, 180], seed: 1, locale: "en",
    actions: { point: ["Space"], die: ["X"], language: ["L"] },
  },
  start: "play",
  scenes: { play, over },
  assets: {
    sounds: {},
    locales: { en: "assets/locales/en.json", pt: "assets/locales/pt.json" },
    ui: { hud: "assets/ui/hud.rml", over: "assets/ui/over.rml" },
  },
});
```

`assets/locales/en.json`:

```json
{
  "hud": { "score": "SCORE {n}", "best": "BEST {n}", "keys": "SPACE score  X die  L language" },
  "over": {
    "title": "GAME OVER",
    "result": { "zero": "NO POINTS", "one": "1 POINT", "other": "{count} POINTS" },
    "again": "PLAY AGAIN"
  }
}
```

`assets/locales/pt.json`:

```json
{
  "hud": { "score": "PONTOS {n}", "best": "RECORDE {n}", "keys": "ESPACO pontua  X morre  L idioma" },
  "over": {
    "title": "FIM DE JOGO",
    "result": { "zero": "NENHUM PONTO", "one": "1 PONTO", "other": "{count} PONTOS" },
    "again": "JOGAR DE NOVO"
  }
}
```

`assets/ui/hud.rml`:

```xml
<rml>
<head><link type="text/rcss" href="game.rcss"/></head>
<body class="safe">
  <p class="bar"><span id="score">SCORE 0</span>   <span id="best">BEST 0</span></p>
  <p class="bar" data-t="hud.keys">SPACE score  X die  L language</p>
</body>
</rml>
```

`assets/ui/over.rml`:

```xml
<rml>
<head><link type="text/rcss" href="game.rcss"/></head>
<body class="safe">
  <div class="panel">
    <h1 data-t="over.title">GAME OVER</h1>
    <p id="result">0 POINTS</p>
    <button id="again" autofocus data-t="over.again">PLAY AGAIN</button>
  </div>
</body>
</rml>
```

`assets/ui/game.rcss`: the one from the [UI page](ui.html#complete-example).

`test/main.test.ts`:

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

export function bestScoreIsSaved() {
  const run = simulate(game, { seconds: 0.5, inputs: { point: [0, 2, 4], die: [6] } });
  run.expect.scene("over");
  run.expect.text("over", "result").equals("3 POINTS");
  if (run.saves.main.best !== 3) throw new Error(`best ${run.saves.main.best}`);
}

export function aBetterOldBestIsKept() {
  const run = simulate(game, { seconds: 0.5, saves: { main: { best: 9 } }, inputs: { point: [0], die: [2] } });
  if (run.saves.main.best !== 9) throw new Error("best overwritten");
  run.expect.text("over", "result").equals("1 POINT");
}

export function languageSwitchPersists() {
  const run = simulate(game, { seconds: 0.5, inputs: { language: [0], point: [2] } });
  if (run.settings.locale !== "pt") throw new Error(`locale ${run.settings.locale}`);
  run.expect.text("hud", "score").equals("PONTOS 1");
}
```

## Step by step

### 1. Save and load values

```ts
ctx.save.set("best", 12);                              // writes the slot file now
const best = ctx.save.get("best", 0);                  // fallback when missing; typed by it
const unlocked = ctx.save.get<string[]>("unlocked", []);
ctx.save.set("unlocked", [...unlocked, "forest"]);
if (ctx.save.has("best")) ctx.save.remove("best");
ctx.save.keys();
ctx.save.clear();
```

Values are JSON-safe: numbers, strings, booleans, null, arrays, plain objects (a copy is stored).
Each write is atomic (temp file + rename, the previous file kept as `.bak`; a damaged file falls
back to its `.bak` with a logged warning).

### 2. Save slots

```ts
const slot = ctx.save.slot("slot2");                   // 1..32 letters, digits, '-' or '_'
slot.set("player", { hp: 3, pos: [10, 20], items: ["key"] });
slot.get("player", null);
slot.clear();
```

`ctx.save` is `slot("main")`. One file per slot.

### 3. Version the data

```ts
config: {
  save: { version: 2, migrate: (data, from) => (from < 2 ? { ...data, gold: data.coins ?? 0 } : data) },
},
```

Bump `version` when the shape changes; `migrate` runs once per older slot when it is read (files
from before versioning are version 1).

### 4. Settings

```ts
ctx.settings.uiScale = 1.5;              // 0.5..3: the UI's dp ratio
ctx.settings.highContrast = true;        // class "high-contrast" on every UI body
ctx.settings.reducedMotion = true;       // camera shake and particles to 1/4
ctx.audio.bus("music").volume = 0.5;     // volumes live on their bus
ctx.window.fullscreen = true;
ctx.input.rebind("fire", ["X", "GamepadA"]);
ctx.locale = "pt";
ctx.settings.reset();                    // all back to defaults, applied and saved
```

All of these persist in `settings.json` (apart from saves) and are applied at boot. Read them back
with `ctx.settings.volumes`, `.locale`, `.fullscreen`, `.bindings`. Honor `reducedMotion` in your
own effects too: `const seconds = ctx.settings.reducedMotion ? 0 : 0.15;`.

### 5. Locales

```ts
// GameTypes: locales: "en" | "pt"
assets: { locales: { en: "assets/locales/en.json", pt: "assets/locales/pt.json" } },
config: { locale: "en" },                // default and fallback (default: the first locale)
```

Locale files are UTF-8 JSON, nested freely.

### 6. Translate in scripts: `ctx.t`

```ts
ctx.t("menu.play");                      // nested keys with dots
ctx.t("hud.score", { n: 42 });           // "SCORE {n}" -> "SCORE 42"
ctx.t("over.result", { count: 1 });      // plural entry { zero?, one, other } picked by count
```

A key missing in the current locale falls back to `config.locale`'s text; missing everywhere it
shows `[key]` (logged once in dev builds).

### 7. Translate in UI: `data-t`

```xml
<h1 data-t="over.title">GAME OVER</h1>
```

The element shows `ctx.t("over.title")` at boot and re-translates whenever `ctx.locale` changes.

## Recipes

### High score

```ts
const best = Math.max(score, ctx.save.get("best", 0));
ctx.save.set("best", best);
```

### Best per level, and "continue"

```ts
const key = `best${level}`;
const best = ctx.save.get(key, 0);
if (!best || moves < best) ctx.save.set(key, moves);
ctx.save.set("level", level + 1);
// title scene:
ctx.go("play", { level: ctx.save.get("level", 0) });
```

(`examples/puzzle/src/scenes/solved.ts`, `start.ts`.)

### Save when the OS may kill the game

```ts
ctx.onPause(() => ctx.save.set("checkpoint", { level: ctx.state.level, hp: ctx.state.hp }));
```

### Language switch button

```ts
ctx.ui.on("settings", "lang", "click", () => {
  const all = ["en", "pt", "ar"] as const;
  ctx.locale = all[(all.indexOf(ctx.locale ?? "en") + 1) % all.length];
});
```

```json
{ "settings": { "languageName": "ENGLISH" } }
```

```xml
<button id="lang" data-t="settings.languageName">ENGLISH</button>
```

Each locale file names its own language, so the button always shows the current one.

### Accessibility toggles

```ts
ctx.ui.on("settings", "motion", "click", () => {
  ctx.settings.reducedMotion = !ctx.settings.reducedMotion;
  ctx.ui.set("settings", "motion", ctx.settings.reducedMotion ? "ON" : "OFF");
});
```

### Controls menu (rebinding)

```ts
ctx.input.rebind("fire", ["X", "GamepadA"]);    // replaces config.actions' list for "fire"
ctx.input.bindings("fire");                      // ["X", "GamepadA"]
```

## Gotchas

- Text you wrote with `ctx.ui.set` or into world text does not re-translate on a locale change:
  set it again after changing `ctx.locale`. Only `data-t` elements update themselves.
- Don't put `data-t` and a script-set `id` on the same element: give the changing part its own
  `<span id>`.
- `save.set` writes the file on every call: fine for events (a level won), not every frame.
- Saves and settings are separate files: `ctx.settings.reset()` never touches saves, `save.clear()`
  never touches settings.
- Tests start from empty in-memory saves and settings: seed them with
  `simulate(game, { saves: { main: { level: 1 } }, settings: { locale: "pt", reducedMotion: true } })`,
  read `run.saves`, `run.locale`, `run.settings`.

## See also

- [Game UI](ui.html) for the settings screen.
- [Text and languages](text.html) for fonts per script, RTL and CJK.
- [Audio](audio.html) for buses.
- `examples/puzzle` (seven locales, best per level), `examples/flappy` (best score, settings,
  language switch).
- In `alpha.d.ts`: `Save`, `SaveSlot`, `SaveConfig`, `Settings`, `Context.t`, `Context.locale`.
