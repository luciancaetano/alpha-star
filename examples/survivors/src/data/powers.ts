// Every weapon: how it fires (kind), what its hits do (mod), its icon, the passive it evolves with
// and its numbers. Texts live in assets/locales (weapon.<id>.name / desc / evo).
import type { Color, HeroId, Mod, PassiveId, WeaponId, WeaponKind } from "../types";
import { BASE, glyph } from "./glyphs";

export interface WeaponDef {
  kind: WeaponKind; mod: Mod; icon: string; evoIcon: string; needs: PassiveId; color: Color;
  /** A signature weapon: only this hero gets it (it starts with it). */
  hero?: HeroId;
  /** Hand-tuned levels with their own texts (weapon.<id>.l1..l6, evoDesc). */
  custom?: boolean;
}

/** Numbers a weapon fires with at `level` (1..6), before the hero's stats. */
export interface WeaponLevel { cd: number; dmg: number; count: number; area: number; pierce: number; life: number }

type Row = [WeaponKind, Mod, keyof typeof BASE, number, Color, PassiveId, [number, number, number, number, number, number], HeroId?];

/** kind, mod, icon base, hue step, color, evolves with, [cd, dmg, count, area, pierce, life], signature of. */
const ROWS: Record<Exclude<WeaponId, "bolt" | "whip" | "lanterns" | "pool" | "spark">, Row> = {
  flare: ["bolt", "blast", "bolt", 1, "#ffb347", "ember", [1, 14, 1, 1, 2, 1.2], "wick"],
  lash: ["whip", "leech", "whip", 6, "#ff5a8a", "vial", [1.2, 20, 2, 1.1, 99, 0.22], "taper"],
  vigil: ["orbit", "slow", "lantern", 4, "#8fd3ff", "frost", [2.5, 12, 2, 1, 99, 4.5], "lumen"],
  cinder: ["rain", "burn", "pool", 1, "#ff7a2a", "hourglass", [1.5, 14, 3, 1, 99, 0], "ash"],
  chime: ["nova", "freeze", "spark", 4, "#bfe8ff", "beacon", [2.2, 16, 1, 1, 99, 0], "vesper"],
  wheel: ["boomerang", "pull", "bolt", 3, "#b6ff7a", "anvil", [1.4, 16, 1, 1, 99, 1.4], "tallow"],
  dart: ["bolt", "crit", "bolt", 2, "#fff2b0", "fang", [0.6, 6, 1, 1, 1, 1]],
  rime: ["bolt", "slow", "bolt", 4, "#9be7ff", "frost", [1.2, 9, 1, 1, 1, 1.2]],
  reaper: ["bolt", "execute", "bolt", 6, "#c58bff", "skull", [1.6, 12, 1, 1, 3, 1.4]],
  comet: ["bolt", "blast", "bolt", 7, "#ff6b6b", "glass", [2.6, 30, 1, 1.2, 1, 1.6]],
  ring: ["ring", "burn", "bolt", 5, "#ff9a3c", "tome", [2.2, 8, 6, 1, 1, 0.9]],
  sickle: ["boomerang", "crit", "whip", 2, "#e0e0ff", "eye", [1.5, 12, 1, 1, 99, 1.3]],
  briar: ["whip", "slow", "whip", 3, "#7adc6f", "thorn", [1.4, 12, 1, 1, 99, 0.22]],
  gilded: ["whip", "gold", "whip", 5, "#ffd36b", "purse", [1.5, 12, 1, 1, 99, 0.22]],
  sunray: ["beam", "burn", "whip", 1, "#ffcf6b", "wick", [2, 14, 1, 1, 99, 0]],
  frostray: ["beam", "freeze", "whip", 4, "#9be7ff", "frost", [2.2, 10, 1, 1, 99, 0]],
  soulray: ["beam", "leech", "whip", 7, "#ff7ad9", "heart", [2, 12, 1, 1, 99, 0]],
  halo: ["orbit", "knock", "lantern", 1, "#ffe08a", "wing", [3.2, 8, 2, 1.1, 99, 3]],
  ghosts: ["orbit", "crit", "lantern", 6, "#b8a6ff", "eye", [3, 9, 1, 1.3, 99, 3]],
  wreath: ["orbit", "burn", "lantern", 2, "#ff8a5a", "thorn", [3.4, 7, 3, 0.8, 99, 3]],
  hearth: ["aura", "burn", "lantern", 3, "#ff9a3c", "salve", [1, 4, 1, 1, 99, 0]],
  chill: ["aura", "slow", "lantern", 5, "#9bd0ff", "shell", [1, 3, 1, 1.1, 99, 0]],
  tar: ["pool", "slow", "pool", 2, "#8a6a4a", "anvil", [2.6, 4, 1, 1.1, 99, 3]],
  puddle: ["pool", "freeze", "pool", 4, "#9be7ff", "hourglass", [3, 4, 1, 1, 99, 2.5]],
  goldwax: ["pool", "gold", "pool", 5, "#ffd36b", "clover", [2.8, 5, 1, 1, 99, 2.5]],
  hail: ["rain", "blast", "pool", 3, "#ffb070", "quill", [1.6, 10, 3, 1, 99, 0]],
  starfall: ["rain", "crit", "pool", 6, "#fff2b0", "crown", [2, 16, 2, 1, 99, 0]],
  bloodspark: ["spark", "leech", "spark", 1, "#ff4d5e", "salve", [1.8, 14, 1, 1, 99, 0]],
  arc: ["chain", "knock", "spark", 2, "#c9a6ff", "prism", [1.6, 12, 3, 1, 99, 0]],
  frostarc: ["chain", "slow", "spark", 5, "#8fd3ff", "echo", [1.8, 9, 3, 1, 99, 0]],
  burst: ["nova", "knock", "spark", 3, "#ffcf6b", "anvil", [2, 12, 1, 1, 99, 0]],
  doom: ["nova", "execute", "spark", 7, "#c58bff", "skull", [3.2, 20, 1, 1.3, 99, 0]],
};

const drawn = (kind: WeaponKind, g: number, needs: PassiveId, color: Color): WeaponDef =>
  ({ kind, mod: "none", icon: "ABCDE"[g], evoIcon: "FGHIJ"[g], needs, color, custom: true });

export const WEAPONS: Record<WeaponId, WeaponDef> = {
  bolt: drawn("bolt", 0, "tome", "#ff9a3c"),
  whip: drawn("whip", 1, "wick", "#ff7a2a"),
  lanterns: drawn("orbit", 2, "wing", "#ffcf6b"),
  pool: drawn("pool", 3, "glass", "#ff8a2a"),
  spark: drawn("spark", 4, "prism", "#c9a6ff"),
  ...Object.fromEntries(Object.entries(ROWS).map(([id, [kind, mod, base, hue, color, needs, , hero]]) =>
    [id, { kind, mod, icon: glyph(BASE[base], hue), evoIcon: glyph(BASE[base] + 5, hue), needs, color, hero }])),
} as Record<WeaponId, WeaponDef>;

export const WEAPON_IDS = Object.keys(WEAPONS) as WeaponId[];

const level = ([cd, dmg, count, area, pierce, life]: number[]): WeaponLevel => ({ cd, dmg, count, area, pierce, life });

/** The shared growth of every non-custom weapon (texts: grow.l2..l6). */
function grow(b: WeaponLevel): WeaponLevel[] {
  const l3 = { ...b, count: b.count + 1, dmg: b.dmg * 1.5 };
  const l5 = { ...l3, count: l3.count + 1, cd: b.cd * 0.85, area: b.area * 1.2 };
  return [b, { ...b, count: b.count + 1 }, l3, { ...l3, count: l3.count + 1 }, l5, { ...l5, count: l5.count + 1, dmg: l5.dmg * 1.3 }];
}

const LEVELS: Record<WeaponId, WeaponLevel[]> = {
  bolt: [[1.1, 10, 1, 1, 1], [1.1, 10, 2, 1, 1], [1.1, 15, 2, 1, 1], [1.1, 15, 3, 1, 2], [0.85, 15, 3, 1, 2], [0.85, 23, 4, 1, 2]]
    .map(([cd, dmg, count, area, pierce]) => ({ cd, dmg, count, area, pierce, life: 1.2 })),
  whip: [[1.35, 14, 1, 1], [1.35, 14, 2, 1], [1.35, 22, 2, 1], [1.35, 22, 2, 1.2], [1.35, 30, 2, 1.2], [1.1, 40, 2, 1.2]]
    .map(([cd, dmg, count, area]) => ({ cd, dmg, count, area, pierce: 99, life: 0.22 })),
  lanterns: [[3, 9, 1, 1, 3], [3, 9, 2, 1, 3], [3, 13, 2, 1.25, 3], [3, 13, 3, 1.25, 3], [3, 13, 3, 1.25, 4.2], [3, 19, 4, 1.25, 4.2]]
    .map(([cd, dmg, count, area, life]) => ({ cd, dmg, count, area, pierce: 99, life })),
  pool: [[2.4, 6, 1, 1, 2.5], [2.4, 6, 2, 1, 2.5], [2.4, 6, 2, 1.3, 2.5], [2.4, 10, 2, 1.3, 2.5], [2.4, 10, 3, 1.3, 3.3], [2.2, 15, 4, 1.3, 3.3]]
    .map(([cd, dmg, count, area, life]) => ({ cd, dmg, count, area, pierce: 99, life })),
  spark: [[1.7, 18, 1, 1], [1.7, 18, 2, 1], [1.7, 28, 2, 1], [1.7, 28, 3, 1], [1.7, 28, 3, 1.4], [1.5, 38, 5, 1.4]]
    .map(([cd, dmg, count, area]) => ({ cd, dmg, count, area, pierce: 99, life: 0.3 })),
  ...Object.fromEntries(Object.entries(ROWS).map(([id, row]) => [id, grow(level(row[6]))])),
} as Record<WeaponId, WeaponLevel[]>;

const EVOLVED: Partial<Record<WeaponId, WeaponLevel>> = {
  bolt: { cd: 0.32, dmg: 30, count: 3, area: 1, pierce: 99, life: 1.4 },
  whip: { cd: 0.9, dmg: 65, count: 2, area: 1.7, pierce: 99, life: 0.25 },
  lanterns: { cd: 0.01, dmg: 26, count: 6, area: 1.5, pierce: 99, life: 1e9 },
  pool: { cd: 1.6, dmg: 22, count: 5, area: 2, pierce: 99, life: 4.5 },
  spark: { cd: 0.9, dmg: 55, count: 8, area: 1.8, pierce: 99, life: 0.3 },
};

/** Evolved from level 6: faster, twice the damage, two more, bigger, pierces all (text: grow.evo). */
const evolve = (l: WeaponLevel): WeaponLevel =>
  ({ cd: l.cd * 0.7, dmg: l.dmg * 2, count: l.count + 2, area: l.area * 1.3, pierce: 99, life: l.life * 1.3 });

export const weaponLevel = (id: WeaponId, lv: number, evo: boolean): WeaponLevel =>
  evo ? EVOLVED[id] ?? evolve(LEVELS[id][5]) : LEVELS[id][Math.min(lv, LEVELS[id].length) - 1];
