// Weapons and passives: names, icon glyphs (assets/fonts/icons.fnt) and per-level numbers.
import type { PassiveId, WeaponId } from "../types";

// Texts live in assets/locales (weapon.<id>.name / evo / evoDesc / l1..l6, passive.<id>.name / desc).
export interface WeaponDef { icon: string; evoIcon: string; needs: PassiveId }

export const WEAPONS: Record<WeaponId, WeaponDef> = {
  bolt: { icon: "A", evoIcon: "F", needs: "tome" },
  whip: { icon: "B", evoIcon: "G", needs: "wick" },
  lanterns: { icon: "C", evoIcon: "H", needs: "wing" },
  pool: { icon: "D", evoIcon: "I", needs: "glass" },
  spark: { icon: "E", evoIcon: "J", needs: "prism" },
};

export interface PassiveDef { icon: string }
export const PASSIVES: Record<PassiveId, PassiveDef> = {
  wick: { icon: "a" },
  tome: { icon: "b" },
  glass: { icon: "c" },
  prism: { icon: "d" },
  wing: { icon: "e" },
  heart: { icon: "f" },
  lode: { icon: "g" },
};

export const WEAPON_IDS = Object.keys(WEAPONS) as WeaponId[];
export const PASSIVE_IDS = Object.keys(PASSIVES) as PassiveId[];

/** Numbers a weapon fires with at `level` (1..6), before the hero's stats. */
export interface WeaponLevel { cd: number; dmg: number; count: number; area: number; pierce: number; life: number }

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
};

const EVOLVED: Record<WeaponId, WeaponLevel> = {
  bolt: { cd: 0.32, dmg: 30, count: 3, area: 1, pierce: 99, life: 1.4 },
  whip: { cd: 0.9, dmg: 65, count: 2, area: 1.7, pierce: 99, life: 0.25 },
  lanterns: { cd: 0.01, dmg: 26, count: 6, area: 1.5, pierce: 99, life: 1e9 },
  pool: { cd: 1.6, dmg: 22, count: 5, area: 2, pierce: 99, life: 4.5 },
  spark: { cd: 0.9, dmg: 55, count: 8, area: 1.8, pierce: 99, life: 0.3 },
};

export const weaponLevel = (id: WeaponId, level: number, evo: boolean): WeaponLevel =>
  evo ? EVOLVED[id] : LEVELS[id][Math.min(level, LEVELS[id].length) - 1];
