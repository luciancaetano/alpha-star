// Icon glyphs of assets/fonts/icons.fnt / bigicons.fnt. Drawn ones: A-E weapons, F-J evolved,
// a-g passives. Recolored ones (tools/process-art.py `tinted_glyphs`): base glyph g in hue step h.

/** Base glyphs in font order: 0-4 weapons (bolt whip lantern pool spark), 5-9 evolved, 10-16 passives. */
export const BASE = { bolt: 0, whip: 1, lantern: 2, pool: 3, spark: 4, wick: 10, tome: 11, glass: 12, prism: 13, wing: 14, heart: 15, lode: 16 };

/** Glyph `g` recolored by hue step `h` (1-7, 45 degrees each). */
export const glyph = (g: number, h: number) => String.fromCharCode(0x100 + g * 8 + h);
