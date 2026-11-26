// Passives (icon glyphs of assets/fonts/icons.fnt) and the weapons re-exported from powers.ts.
// Texts live in assets/locales (passive.<id>.name / desc); what each does is in game/run.ts `stats`.
import type { PassiveId } from "../types";
import { BASE, glyph } from "./glyphs";

export { WEAPON_IDS, WEAPONS, type WeaponDef, type WeaponLevel, weaponLevel } from "./powers";

export interface PassiveDef { icon: string }

const tinted = (base: keyof typeof BASE, hue: number): PassiveDef => ({ icon: glyph(BASE[base], hue) });

export const PASSIVES: Record<PassiveId, PassiveDef> = {
  wick: { icon: "a" }, tome: { icon: "b" }, glass: { icon: "c" }, prism: { icon: "d" },
  wing: { icon: "e" }, heart: { icon: "f" }, lode: { icon: "g" },
  ember: tinted("wick", 1), frost: tinted("glass", 4), fang: tinted("prism", 7), eye: tinted("glass", 2),
  vial: tinted("heart", 5), shell: tinted("heart", 3), hourglass: tinted("tome", 2), quill: tinted("wing", 3),
  clover: tinted("lode", 3), crown: tinted("tome", 5), purse: tinted("lode", 1), anvil: tinted("wick", 5),
  thorn: tinted("wing", 7), salve: tinted("heart", 2), echo: tinted("prism", 3), skull: tinted("wick", 4),
  beacon: tinted("glass", 6), omen: tinted("tome", 7),
};

export const PASSIVE_IDS = Object.keys(PASSIVES) as PassiveId[];
