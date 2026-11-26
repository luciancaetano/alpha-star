# Text and languages

World text and the game UI draw any Unicode text through one pipeline: UTF-8 checked where it
enters, paragraphs ordered by the Unicode bidi algorithm (SheenBidi), TrueType/OpenType runs
shaped by HarfBuzz (ligatures, Arabic joining, Devanagari and Thai clusters), characters the
font lacks taken from its fallback fonts, and glyphs rasterized at the screen's real resolution.

## World text

```ts
ctx.spawn({ tags: ["sign"], pos: [160, 40],
  shape: { text: "مرحبا Hello 你好", font: "main", size: 16, color: "#ffffff" } });
ctx.spawn({ tags: ["story"], pos: [160, 120],
  shape: { text: ctx.t("intro"), font: "main", size: 12, width: 200, align: "left", dir: "auto" } });
```

- `width` wraps lines at that many pixels (0, the default, breaks only at `"\n"`). Lines break
  after spaces and between CJK characters, never before closing punctuation (`。、）」` and small
  kana) or after opening brackets. A word longer than the line is cut where it overflows.
- `dir`: `"auto"` (default) takes the paragraph direction from its first strong letter (Arabic,
  Hebrew: right to left), `"ltr"` / `"rtl"` force it. Mixed runs (a Latin name in an Arabic
  sentence) keep their own order either way.
- `align` places the lines on `pos`; the block of lines is vertically centered on it.
- Text that is not valid Unicode, typically half an emoji left by slicing a JS string
  (`"😀".slice(0, 1)`), is an error naming the byte where it breaks. At most 4096 bytes.

## Fonts per language

One font rarely covers every script. Give the main font `fallbacks`: other `assets.fonts`
names tried in order for each character it lacks.

```ts
fonts: {
  main: { file: "assets/fonts/NotoSans-Subset.ttf", license: "assets/fonts/OFL.txt",
          fallbacks: ["arabic", "hebrew", "devanagari", "cjk"] },
  arabic: { file: "assets/fonts/NotoSansArabic-Subset.ttf", license: "assets/fonts/OFL.txt" },
  hebrew: { file: "assets/fonts/NotoSansHebrew-Subset.ttf", license: "assets/fonts/OFL.txt" },
  devanagari: { file: "assets/fonts/NotoSansDevanagari-Subset.ttf", license: "assets/fonts/OFL.txt" },
  cjk: { file: "assets/fonts/NotoSansSC-Subset.otf", license: "assets/fonts/OFL-NotoSansCJK.txt" },
},
```

| Script | Noto font |
|---|---|
| Latin, Cyrillic, Greek | Noto Sans |
| Arabic, Persian, Urdu | Noto Sans Arabic |
| Hebrew | Noto Sans Hebrew |
| Devanagari (Hindi, Marathi, Nepali) | Noto Sans Devanagari |
| Thai | Noto Sans Thai |
| Chinese (simplified / traditional), Japanese, Korean | Noto Sans SC / TC / JP / KR |

- Marks, joiners and spaces stay with the font of the text around them, so a word is never
  split across fonts.
- Fallbacks must be `.ttf`/`.otf`; a BMFont or the built-in font cannot have any. Those draw what
  they lack as a box.
- What no font covers draws as a box, and the log says once per font and Unicode block which
  character was missing (e.g. `U+0E01 (block U+0E00..U+0E7F)`): add a font for it.
- Line height and baseline come from the main font.
- Glyphs are cached per font and size in atlases of at most 2048 x 2048; a text needing more
  starts the atlas over, so large CJK sets work with bounded memory.

### Keep packs small

Full CJK fonts are 8+ MB. Cut each font to the characters your game shows (its locale files)
with `tools/subset_fonts.py` from the engine repository (needs `pip install fonttools`); it keeps
every OpenType feature, so shaping still works:

```
python tools/subset_fonts.py NotoSansSC-Regular.otf assets/fonts/NotoSansSC-Subset.otf --text-file assets/locales/zh.json
```

`examples/puzzle` ships seven languages (English, Portuguese, Russian, Arabic, Hebrew, Hindi,
Chinese) in about 200 KB of fonts; press L to cycle them.

### Licensing

Noto fonts are under the SIL Open Font License 1.1: free to ship in a game, subset or not, as
long as the license goes with them. Set each font's `license` to its OFL text and `alpha export`
adds it to `THIRD_PARTY_NOTICES.txt`. Use the license file of the font's own project (Noto CJK
has its own).

## Game UI

UI text goes through the same fonts and shaping. The `font-family` is the `assets.fonts` name;
fallbacks apply. RCSS `direction: rtl` (or `ltr`) sets the paragraph direction of an element's
text; without it each line takes its direction from its first strong letter. RmlUi wraps at
spaces; for CJK paragraphs set `word-break: break-all` so lines can break between characters.
Font effects (`font-effect: shadow/outline`) are not drawn.
