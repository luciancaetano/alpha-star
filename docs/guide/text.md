# Text and languages

**What you'll build:** a title, a score counter, a wrapped paragraph and "+10" popups that float
up and fade, all as world text.

World text is a `text` shape on an entity: it lives in the world (moves with the camera, sorts by
`z`). For menus and HUD panels use the [game UI](ui.html); both draw through the same fonts and
shaping, rasterized at the screen's real resolution so text stays sharp.

## Complete example

`src/main.ts` (no font files needed: without `assets.fonts` the built-in 8x8 font draws it):

```ts
import { define, type Context } from "alpha";

interface Signs {
  actions: "hit";
  sounds: never;
  images: never;
  tags: { title: {}; score: {}; popup: {}; story: {} };
  scenes: { play: undefined };
  ui: {};
}

const { scene, game } = define<Signs>();

function popup(ctx: Context<Signs, { score: number }>, x: number, y: number, points: number) {
  const p = ctx.spawn({ tags: ["popup"], pos: [x, y], z: 10,
    shape: { text: `+${points}`, size: 16, color: "#ffd166" } });
  ctx.tween(p, { to: { y: y - 30, alpha: 0 }, seconds: 0.8, ease: "quadOut", onDone: () => ctx.destroy(p) });
}

const play = scene("play", {
  state: () => ({ score: 0 }),
  enter(ctx) {
    ctx.spawn({ tags: ["title"], pos: [160, 24], shape: { text: "SIGNS", size: 24, color: "#ffffff" } });
    ctx.spawn({ tags: ["score"], pos: [8, 8], shape: { text: "SCORE 0", size: 8, align: "left" } });
    ctx.spawn({ tags: ["story"], pos: [160, 110], shape: {
      text: "Press Space to score. Long lines wrap at the width you give.",
      size: 8, width: 200, color: "#c2c3c7" } });
  },
  update(ctx) {
    if (!ctx.input.pressed("hit")) return;
    ctx.state.score += 10;
    ctx.find("score").text = `SCORE ${ctx.state.score}`;
    popup(ctx, ctx.random.range(60, 260), 70, 10);
  },
});

export default game({
  config: { title: "Signs", size: [320, 180], background: "#1d2b53", seed: 1,
    actions: { hit: ["Space", "MouseLeft", "Touch"] } },
  start: "play",
  scenes: { play },
  assets: { sounds: {} },
});
```

`test/main.test.ts`:

```ts
import { simulate } from "alpha/test";
import game from "../src/main";

export function popupsFadeAway() {
  simulate(game, { seconds: 0.5, inputs: { hit: [0, 5] } }).expect.count("popup").equals(2);
  const run = simulate(game, { seconds: 2, inputs: { hit: [0, 5] } });
  run.expect.count("popup").equals(0);
  run.expect.noErrors();
}
```

## Step by step

### 1. A line of text

```ts
ctx.spawn({ tags: ["label"], pos: [160, 20], shape: { text: "READY", size: 16, color: "#ffffff" } });
```

`size` is pixels high (1..256, default 16); `color` defaults to white. With the built-in font, the
size snaps to the nearest multiple of 8.

### 2. Change it, color it, scale it

```ts
const label = ctx.find("label");
label.text = "GO!";
label.tint = "#ff0000";          // multiplies color
label.scale = 2;                 // text scales its size
ctx.tween(label, { to: { scale: 1 }, seconds: 0.3, ease: "backOut" });
```

`entity.text` throws on entities that are not text. `alpha`, `tint`, `blend`, `rotation` work like
any shape.

### 3. Alignment and wrapping

```ts
ctx.spawn({ tags: ["sign"], pos: [8, 8], shape: { text: "LEFT", align: "left" } });
ctx.spawn({ tags: ["sign"], pos: [160, 90], shape: { text: ctx.t("intro"), size: 12, width: 200, align: "left" } });
```

`pos` is the anchor: `align` (default `"center"`) picks which end of the lines sits on it, and the
block of lines is centered vertically on it. `width` wraps at that many pixels (0 = only at
`"\n"`). Lines break after spaces and between CJK characters, never before closing punctuation
(`。、）`). A word longer than the line is cut where it overflows.

### 4. Text over an entity

```ts
const hp = ctx.spawn({ tags: ["hp"], pos: [0, -14], parent: ctx.find("player"), shape: { text: "3", size: 8 } });
hp.text = "2";
```

A child's `pos` is local to the parent: the label follows it and dies with it.

### 5. A font

```ts
// GameTypes: fonts: "pixel" | "main"
assets: {
  fonts: {
    pixel: { file: "assets/fonts/PressStart2P-Regular.ttf", pixelMode: true, license: "assets/fonts/OFL.txt" },
    main: { file: "assets/fonts/NotoSans-Subset.ttf", license: "assets/fonts/OFL.txt" },
  },
},
// a scene
ctx.spawn({ tags: ["label"], pos: [160, 20], shape: { text: "READY", font: "pixel", size: 16 } });
```

- `.ttf` / `.otf`, or a BMFont `.fnt` (its PNG pages are packed too). Bitmap fonts draw at whole
  multiples of their size.
- `pixelMode: true` renders TrueType glyphs monochrome: pixel fonts stay crisp.
- `license` names the font's license file; `alpha export` copies it and adds it to
  `THIRD_PARTY_NOTICES.txt`.
- The same name is the `font-family` in `.rcss`.

### 6. Any language: fallbacks

One font rarely covers every script. `fallbacks` are other `assets.fonts` names tried in order for
each character the main font lacks:

```ts
fonts: {
  main: { file: "assets/fonts/NotoSans-Subset.ttf", license: "assets/fonts/OFL.txt",
          fallbacks: ["arabic", "cjk"] },
  arabic: { file: "assets/fonts/NotoSansArabic-Subset.ttf", license: "assets/fonts/OFL.txt" },
  cjk: { file: "assets/fonts/NotoSansSC-Subset.otf", license: "assets/fonts/OFL-NotoSansCJK.txt" },
},
// a scene
ctx.spawn({ tags: ["sign"], pos: [160, 40], shape: { text: "مرحبا Hello 你好", font: "main", size: 16 } });
```

Result: Arabic is joined and ordered right to left, Latin and Chinese from their fonts, all in one
line. TrueType/OpenType text is shaped by HarfBuzz (ligatures, Arabic joining, Devanagari and Thai
clusters) and ordered by the Unicode bidi algorithm.

| Script | Noto font |
|---|---|
| Latin, Cyrillic, Greek | Noto Sans |
| Arabic, Persian, Urdu | Noto Sans Arabic |
| Hebrew | Noto Sans Hebrew |
| Devanagari (Hindi, Marathi, Nepali) | Noto Sans Devanagari |
| Thai | Noto Sans Thai |
| Chinese (simplified / traditional), Japanese, Korean | Noto Sans SC / TC / JP / KR |

### 7. Direction

```ts
ctx.spawn({ tags: ["sign"], pos: [300, 40], shape: { text: "שלום", font: "main", align: "right", dir: "rtl" } });
```

`dir: "auto"` (default) takes the paragraph direction from its first strong letter; `"ltr"` /
`"rtl"` force it. Mixed runs (a Latin name in an Arabic sentence) keep their own order either way.

### 8. In the UI

```css
body { font-family: main; }           /* fallbacks apply here too */
.story { direction: rtl; }            /* paragraph direction of an element */
.cjk { word-break: break-all; }       /* RmlUi wraps at spaces; let CJK break between characters */
```

## Recipes

### Floating damage numbers

The `popup` function in the complete example: spawn, tween `y` and `alpha`, destroy in `onDone`.

### Typewriter dialog

```ts
export default scene("talk", {
  state: () => ({ shown: 0 }),
  enter(ctx) {
    ctx.spawn({ tags: ["dialog"], pos: [20, 140], shape: { text: "", font: "main", size: 10, width: 280, align: "left" } });
  },
  update(ctx) {
    const chars = Array.from(ctx.t("intro"));          // code points: never split an emoji
    if (ctx.state.shown < chars.length) {
      ctx.state.shown += 30 * ctx.dt;                  // 30 characters per second
      ctx.find("dialog").text = chars.slice(0, Math.floor(ctx.state.shown)).join("");
    }
    if (ctx.input.pressed("talk")) ctx.state.shown = chars.length;   // skip
  },
});
```

### Localized world text

```ts
ctx.spawn({ tags: ["language"], pos: [160, 170], shape: { text: ctx.t("hud.language"), font: "main", size: 10 } });
// after ctx.locale changes, world text does not update itself:
ctx.find("language").text = ctx.t("hud.language");
```

(`examples/puzzle/src/scenes/play.ts`.)

### Keep packs small

Full CJK fonts are 8+ MB. Cut each font to the characters your game shows with
`tools/subset_fonts.py` from the engine repository (`pip install fonttools`); OpenType features
are kept, so shaping still works:

```
python tools/subset_fonts.py NotoSansSC-Regular.otf assets/fonts/NotoSansSC-Subset.otf --text-file assets/locales/zh.json
```

`examples/puzzle` ships seven languages (English, Portuguese, Russian, Arabic, Hebrew, Hindi,
Chinese) in about 200 KB of fonts; press L to cycle them.

## Gotchas

- Slicing a JS string can cut an emoji in half (`"😀".slice(0, 1)`): invalid Unicode is an error
  naming where it breaks. Slice `Array.from(text)` instead. At most 4096 bytes per text.
- A character no font has draws as a box, and the log names it once per font and Unicode block
  (`U+0E01 (block U+0E00..U+0E7F)`): add a fallback font for it.
- Fallbacks only work on `.ttf` / `.otf` fonts; BMFont and the built-in font cannot have any.
- Marks, joiners and spaces stay with the font around them; line height and baseline come from
  the main font.
- Text is rasterized at game size (not window size) while `bloom` or `pixelate` is on.
- Licenses: Noto fonts are SIL OFL 1.1, free to ship with the license file. Use each font project's
  own license file (Noto CJK has its own).

## See also

- [Game UI](ui.html) for HUDs and menus.
- [Saves, settings, i18n](saves.html) for `ctx.t`, locales and plural forms.
- `examples/puzzle` (seven languages, fallbacks), `examples/topdown` and `examples/shmup` (built-in font).
- In `alpha.d.ts`: `TextShape`, `FontAsset`, `Entity.text`.
