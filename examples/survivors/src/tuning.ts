// Gameplay numbers. Change the feel here, not in the systems.
import type { EnemyKind, HeroId } from "./types";

// DESIGN resolution (the virtual canvas the art and UI are drawn for), NOT the window size: the engine
// scales it to any window, monitor or phone (config.window). Changing it shrinks/grows everything.
export const VIEW = { width: 384, height: 216 };
export const RUN = { seconds: 15 * 60, maxEnemies: 260, maxGems: 220, despawn: 140 };  // despawn: px past the view corners before a foe is brought back ahead
export const HERO = { speed: 72, hp: 100, radius: 6, iframes: 0.3, crowd: 5, light: 150 };
export const PICKUP = { magnet: 50, grab: 8, pull: 240, cakeHeal: 30, bellRadius: 600 };
export const SLOTS = { weapons: 5, passives: 5, weaponMax: 6, passiveMax: 5 };
export const GEM = { mid: 5, big: 25 };
export const BRAZIER = { every: 18, max: 4, hp: 1 };
export const GROUND = { tile: 128, cell: 112 };

/** XP needed to leave `level` (1-based). */
export const xpFor = (level: number) => 4 + (level - 1) * 4 + Math.max(0, level - 20) * 5;

export interface EnemyDef { hp: number; speed: number; dmg: number; xp: number; radius: number; size: number }
export const ENEMIES: Record<EnemyKind, EnemyDef> = {
  moth: { hp: 6, speed: 44, dmg: 3, xp: 1, radius: 5, size: 16 },
  shade: { hp: 14, speed: 34, dmg: 5, xp: 1, radius: 6, size: 16 },
  wisp: { hp: 9, speed: 66, dmg: 4, xp: 2, radius: 5, size: 16 },
  ghoul: { hp: 42, speed: 25, dmg: 9, xp: 3, radius: 8, size: 20 },
  gargoyle: { hp: 380, speed: 40, dmg: 18, xp: 30, radius: 10, size: 28 },
  queen: { hp: 2600, speed: 33, dmg: 25, xp: 120, radius: 20, size: 56 },
  snuffer: { hp: 999999, speed: 130, dmg: 999, xp: 0, radius: 14, size: 48 },
};

export interface HeroDef { cost: number; weapon: "bolt" | "whip" | "lanterns"; tint: string }
export const HEROES: Record<HeroId, HeroDef> = {
  wick: { cost: 0, weapon: "bolt", tint: "#ffd9a0" },
  taper: { cost: 400, weapon: "whip", tint: "#e9b8ff" },
  lumen: { cost: 900, weapon: "lanterns", tint: "#9be7ff" },
};
