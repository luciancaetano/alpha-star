# Text and Fonts

> summary: World text shapes, font assets (TTF/OTF, BMFont .fnt, built-in 8x8), any language (fallback fonts, right-to-left `dir`, wrapping `width`, CJK), pixelMode for fonts and font licenses; read when showing text in the world, choosing fonts for world text and UI, or adding a language.

## API

```ts
// types.ts (optional): fonts: "pixel";
// main.ts assets
fonts: { pixel: { file: "assets/fonts/PressStart2P-Regular.ttf", license: "assets/fonts/OFL.txt" } },
type FontAsset = string | { file: string; pixelMode?: boolean; license?: string; fallbacks?: F[] };

// world text shape
{ text: string, font?: F, size?: number /* 1..256, default 16 */, color?: Color /* "#ffffff" */,
  align?: "left" | "center" | "right" /* default center */,
  dir?: "auto" | "ltr" | "rtl" /* default auto: first strong letter */,
  width?: number /* wrap at this many px; 0 = only at "\n" */, tint?, alpha?, blend? }
e.text = "Score 3";     // world text entities only; throws for others
e.scale = 2;            // text scales its size
```

## Example (examples/platformer/src/prefabs/effects.ts)

```ts
export function popup(at: Vec2): Spec {
  return { tags: ["popup"], pos: [at[0], at[1] - 10], layer: "fx",
           shape: { text: "+1", font: "pixel", size: 8, color: "#ffe14d" } };
}
```
Without `font`: `shape: { text: "+1", size: 8, color: "#ffe14d" }` uses the built-in 8x8 font.

## Rules and gotchas

- The font name is both the `font` of world text and the `font-family` in `.rcss` (ui.md).
- Formats: `.ttf`, `.otf`, AngelCode BMFont `.fnt` (text or XML; its PNG pages are packed too).
- No fonts at all: UI and world text use the built-in 8x8 font (public domain); it draws at the nearest whole multiple of 8, so use sizes in multiples of 8.
- Text is rasterized at the screen's real resolution (window scale, high-DPI, camera zoom): sharp at any size. Bitmap fonts draw at whole multiples of their size.
- `pixelMode: true` on a font renders TrueType glyphs monochrome (pixel fonts stay crisp; for an 8 px grid font, sizes where size x window scale is a multiple of 8 look best).
- `pos` is the anchor; `align` picks which end of the lines sits on it; the block of lines is vertically centered.
- Any Unicode text, world and UI alike: TTF/OTF text is shaped (ligatures, Arabic joining, Devanagari), mixed right-to-left / left-to-right runs are ordered by Unicode bidi, `"\n"` breaks lines, `width` wraps after spaces and between CJK characters (never before closing punctuation like 。、）).
- Invalid Unicode (an unpaired surrogate, e.g. `"😀".slice(0, 1)`) is an error naming the byte; at most 4096 bytes.
- One font rarely has every script: `main: { file, fallbacks: ["arabic", "cjk"] }` lists other assets.fonts names tried in order per character (TTF/OTF only). What no font has draws as a box and logs once per font and Unicode block (`U+0E01 (block U+0E00..U+0E7F)`): add a fallback for it. BMFont / built-in fonts draw boxes for what they lack.
- Noto per script: Noto Sans (Latin, Cyrillic, Greek), Noto Sans Arabic, Hebrew, Devanagari, Thai, Noto Sans SC/TC/JP/KR (CJK). All SIL OFL: set `license` to the OFL file (Noto CJK has its own).
- CJK fonts are 8+ MB: cut them to the locale's characters with the engine repo's `tools/subset_fonts.py FONT OUT --text-file assets/locales/zh.json` (keeps OpenType features). `examples/puzzle` ships 7 languages in ~200 KB of fonts.
- UI: RCSS `direction: rtl` sets the paragraph direction; CJK paragraphs need `word-break: break-all` to wrap; `font-effect` is not drawn.
- `license`: `alpha export` copies the file and adds its text to `THIRD_PARTY_NOTICES.txt`. The font's own license matters (SIL OFL is fine; a bitmap font made from a font follows that font's license). It must be a file path, else an error.
- While `bloom` or `pixelate` postfx is on, world text is rasterized at game size.
