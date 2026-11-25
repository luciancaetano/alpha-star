# Text and Fonts

> summary: World text shapes, font assets (TTF/OTF, BMFont .fnt, built-in 8x8), pixelMode for fonts and font licenses; read when showing text in the world or choosing fonts for world text and UI.

## API

```ts
// types.ts (optional): fonts: "pixel";
// main.ts assets
fonts: { pixel: { file: "assets/fonts/PressStart2P-Regular.ttf", license: "assets/fonts/OFL.txt" } },
type FontAsset = string | { file: string; pixelMode?: boolean; license?: string };

// world text shape
{ text: string, font?: F, size?: number /* 1..256, default 16 */, color?: Color /* "#ffffff" */,
  align?: "left" | "center" | "right" /* default center */, tint?, alpha?, blend? }
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
- `pos` is the anchor; `align` picks which end sits on it; vertically centered.
- World text draws ASCII + Latin-1 only (others as `?`). RmlUi UI text draws whatever its fonts have.
- `license`: `alpha export` copies the file and adds its text to `THIRD_PARTY_NOTICES.txt`. The font's own license matters (SIL OFL is fine; a bitmap font made from a font follows that font's license). It must be a file path, else an error.
- While `bloom` or `pixelate` postfx is on, world text is rasterized at game size.
