// The run's JSON state (survives hot reload) and the hero's derived stats: pure functions.
import type { System } from "alpha";
import { HERO, HEROES, PICKUP, xpFor } from "../tuning";
import type { Candle, Ctx, HeroId, PassiveId, ShopId, WeaponId } from "../types";

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
  /** Showcase run (game/demo.ts): autopilot, no damage, no menus. */
  demo: boolean;
  /** The visible world last step (ctx.camera.view), for tests and MCP. */
  view: number[];
}
export type Play = Ctx<RunState>;
/** One piece of the play scene's per-step logic (scenes/play.ts `systems`). */
export type PlaySystem = System<Candle, RunState>;

export function newRun(hero: HeroId, shop: Partial<Record<ShopId, number>>): RunState {
  return {
    hero, time: 0, kills: 0, gold: 0, xp: 0, level: 1, hp: HERO.hp + (shop.vigor ?? 0) * 10, iframes: 0,
    weapons: [{ id: HEROES[hero].weapon, level: 1, evo: false, cd: 0.4 }], passives: {},
    mode: "play", pending: 0, chests: 0, offers: [], spawnCd: 0, events: [], brazierCd: 6, shop, facing: 1, over: false, demo: false, view: [],
  };
}

export interface Stats {
  might: number; cooldown: number; area: number; amount: number; speed: number;
  maxHp: number; regen: number; magnet: number; growth: number; greed: number; armor: number; duration: number;
  /** Chances 0..1 per hit; critMul multiplies a critical hit. */
  crit: number; critMul: number; burn: number; slow: number;
  /** HP healed per hit; knockback multiplier; non-boss foes below this share of their HP die. */
  leech: number; knock: number; execute: number;
  /** Damage to each foe touching the hero; chance a weapon fires again at once. */
  thorns: number; echo: number;
  /** Drop chance multiplier; projectile speed multiplier; foes (and their xp/gold) multiplier. */
  luck: number; shotSpeed: number; omen: number;
}

type Level = (id: PassiveId) => number;

function core(s: RunState, p: Level, m: (id: ShopId) => number): Stats {
  const b = HEROES[s.hero].bonus;
  return {
    might: 1 + 0.1 * p("wick") + 0.05 * m("might") + (b.might ?? 0),
    cooldown: Math.max(0.4, 1 - 0.08 * p("tome") - (b.cooldown ?? 0)),
    area: 1 + 0.1 * p("glass") + (b.area ?? 0),
    amount: p("prism"),
    speed: HERO.speed * (1 + 0.1 * p("wing") + 0.05 * m("swift") + (b.speed ?? 0)),
    maxHp: HERO.hp + 20 * p("heart") + 10 * m("vigor") + (b.maxHp ?? 0),
    regen: 0.3 * p("heart") + 0.1 * m("haste") + 0.5 * p("salve"),
    magnet: PICKUP.magnet * (1 + 0.3 * p("lode") + 0.1 * m("magnet") + 0.1 * p("beacon")),
    growth: (1 + 0.06 * m("growth") + 0.08 * p("crown") + 0.08 * p("beacon")) * (1 + 0.1 * p("omen")),
    greed: (1 + 0.1 * m("greed") + 0.15 * p("purse")) * (1 + 0.1 * p("omen")),
    armor: m("armor") + p("shell") + (b.armor ?? 0),
    duration: 1 + 0.15 * p("hourglass") + (b.duration ?? 0),
    ...extra(p, b),
  };
}

function extra(p: Level, b: (typeof HEROES)[HeroId]["bonus"]) {
  return {
    crit: 0.05 * p("fang"), critMul: 2 + 0.3 * p("eye"), burn: 0.08 * p("ember") + (b.burn ?? 0), slow: 0.08 * p("frost"),
    leech: 0.1 * p("vial"), knock: 1 + 0.25 * p("anvil") + (b.knock ?? 0), execute: 0.04 * p("skull"),
    thorns: 8 * p("thorn"), echo: 0.08 * p("echo"), luck: 1 + 0.2 * p("clover"), shotSpeed: 1 + 0.15 * p("quill"),
    omen: 1 + 0.1 * p("omen"),
  };
}

export const stats = (s: RunState): Stats => core(s, (id) => s.passives[id] ?? 0, (id) => s.shop[id] ?? 0);

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
