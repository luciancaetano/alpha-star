// The run's JSON state (survives hot reload) and the hero's derived stats: pure functions.
import { HERO, HEROES, PICKUP, xpFor } from "../tuning";
import type { Ctx, HeroId, PassiveId, ShopId, WeaponId } from "../types";

export interface OwnedWeapon { id: WeaponId; level: number; evo: boolean; cd: number }
export type Offer = { kind: "weapon"; id: WeaponId } | { kind: "passive"; id: PassiveId } | { kind: "gold" } | { kind: "cake" };
export type Mode = "play" | "levelup" | "chest" | "paused";

export interface RunState {
  hero: HeroId; time: number; kills: number; gold: number;
  xp: number; level: number; hp: number; iframes: number;
  weapons: OwnedWeapon[]; passives: Partial<Record<PassiveId, number>>;
  mode: Mode; pending: number; chests: number; offers: Offer[];
  spawnCd: number; events: string[]; brazierCd: number; shop: Partial<Record<ShopId, number>>;
  facing: number; over: boolean;
  /** The visible world last step (ctx.camera.view), for tests and MCP. */
  view: number[];
}
export type Play = Ctx<RunState>;

export function newRun(hero: HeroId, shop: Partial<Record<ShopId, number>>): RunState {
  return {
    hero, time: 0, kills: 0, gold: 0, xp: 0, level: 1, hp: HERO.hp + (shop.vigor ?? 0) * 10, iframes: 0,
    weapons: [{ id: HEROES[hero].weapon, level: 1, evo: false, cd: 0.4 }], passives: {},
    mode: "play", pending: 0, chests: 0, offers: [], spawnCd: 0, events: [], brazierCd: 6, shop, facing: 1, over: false, view: [],
  };
}

export interface Stats {
  might: number; cooldown: number; area: number; amount: number; speed: number;
  maxHp: number; regen: number; magnet: number; growth: number; greed: number; armor: number; duration: number;
}

export function stats(s: RunState): Stats {
  const p = (id: PassiveId) => s.passives[id] ?? 0;
  const m = (id: ShopId) => s.shop[id] ?? 0;
  const hero = s.hero;
  return {
    might: 1 + 0.1 * p("wick") + 0.05 * m("might") + (hero === "taper" ? 0.2 : 0),
    cooldown: Math.max(0.4, 1 - 0.08 * p("tome")),
    area: 1 + 0.1 * p("glass") + (hero === "lumen" ? 0.25 : 0),
    amount: p("prism"),
    speed: HERO.speed * (1 + 0.1 * p("wing") + 0.05 * m("swift") + (hero === "wick" ? 0.1 : 0)),
    maxHp: HERO.hp + 20 * p("heart") + 10 * m("vigor"),
    regen: 0.3 * p("heart") + 0.1 * m("haste"),
    magnet: PICKUP.magnet * (1 + 0.3 * p("lode") + 0.1 * m("magnet")),
    growth: 1 + 0.06 * m("growth"),
    greed: 1 + 0.1 * m("greed"),
    armor: m("armor"),
    duration: 1,
  };
}

/** Adds xp; returns how many levels were gained. */
export function gainXp(s: RunState, amount: number): number {
  s.xp += amount;
  let gained = 0;
  while (s.xp >= xpFor(s.level)) {
    s.xp -= xpFor(s.level);
    s.level += 1;
    gained += 1;
  }
  return gained;
}

export const clock = (seconds: number) => {
  const t = Math.max(0, Math.floor(seconds));
  return `${String(Math.floor(t / 60)).padStart(2, "0")}:${String(t % 60).padStart(2, "0")}`;
};
