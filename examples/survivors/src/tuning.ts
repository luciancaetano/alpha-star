// Gameplay numbers. Change the feel here, not in the systems.
import type { EnemyKind, HeroId, WeaponId } from "./types";

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

/** Hero bonuses add to the stats (game/run.ts): speed is a fraction (+0.1 = 10% faster). */
export interface HeroBonus { might?: number; area?: number; speed?: number; cooldown?: number; armor?: number; maxHp?: number; burn?: number; duration?: number; knock?: number }
/** `weapon`: the hero's signature, only they get it (src/data/powers.ts). */
export interface HeroDef { cost: number; weapon: WeaponId; tint: string; bonus: HeroBonus }
export const HEROES: Record<HeroId, HeroDef> = {
  wick: { cost: 0, weapon: "flare", tint: "#ffd9a0", bonus: { speed: 0.1 } },
  taper: { cost: 400, weapon: "lash", tint: "#e9b8ff", bonus: { might: 0.2 } },
  lumen: { cost: 900, weapon: "vigil", tint: "#9be7ff", bonus: { area: 0.25 } },
  ash: { cost: 1300, weapon: "cinder", tint: "#d0c8c0", bonus: { cooldown: 0.1, burn: 0.1 } },
  vesper: { cost: 1700, weapon: "chime", tint: "#8fb0ff", bonus: { armor: 1, duration: 0.2 } },
  tallow: { cost: 2200, weapon: "wheel", tint: "#b6ff7a", bonus: { maxHp: 40, knock: 0.5, speed: -0.1 } },
};
