// Every name the game uses. The compiler rejects anything not listed here.
import type { Context, Entity, EntitySpec, TagOf, Vec2 } from "alpha";

export type WeaponId = "bolt" | "whip" | "lanterns" | "pool" | "spark"
  // Signatures: one per hero, only that hero gets it (src/data/powers.ts).
  | "flare" | "lash" | "vigil" | "cinder" | "chime" | "wheel"
  | "dart" | "rime" | "reaper" | "comet" | "ring" | "sickle" | "briar" | "gilded" | "sunray" | "frostray" | "soulray"
  | "halo" | "ghosts" | "wreath" | "hearth" | "chill" | "tar" | "puddle" | "goldwax" | "hail" | "starfall"
  | "bloodspark" | "arc" | "frostarc" | "burst" | "doom";
export type PassiveId = "wick" | "tome" | "glass" | "prism" | "wing" | "heart" | "lode"
  | "ember" | "frost" | "fang" | "eye" | "vial" | "shell" | "hourglass" | "quill" | "clover" | "crown" | "purse"
  | "anvil" | "thorn" | "salve" | "echo" | "skull" | "beacon" | "omen";
export type HeroId = "wick" | "taper" | "lumen" | "ash" | "vesper" | "tallow";
/** How a weapon fires (src/game/weapons.ts, areas.ts). */
export type WeaponKind = "bolt" | "ring" | "boomerang" | "whip" | "beam" | "orbit" | "aura" | "pool" | "rain" | "spark" | "chain" | "nova";
/** What a weapon's hit does besides damage (src/game/strike.ts). */
export type Mod = "none" | "burn" | "slow" | "freeze" | "leech" | "crit" | "pull" | "blast" | "execute" | "gold" | "knock";
export type EnemyKind = "moth" | "shade" | "ghoul" | "wisp" | "gargoyle" | "queen" | "snuffer";
export type ShopId = "might" | "armor" | "vigor" | "haste" | "swift" | "magnet" | "growth" | "greed";

/** What a finished run hands to the "over" scene. */
export type RunResult = { hero: HeroId; seconds: number; level: number; kills: number; gold: number; dawn: boolean };

export type HeroSlot = 1 | 2 | 3 | 4 | 5 | 6;
type Card = "icon" | "name" | "tag" | "desc";

export type ItemKind = "coin" | "cake" | "magnet" | "bell" | "chest";
/** Who dealt a hit: a weapon, or thorns (passive). */
export type HitSource = WeaponId | "thorn";
/** A weapon's hit riding on what it left in the world (src/game/strike.ts). */
export interface WeaponHit { dmg: number; mod: Mod; weapon: HitSource }
/** A foe: stats, timers (seconds left / until state.time) and per-weapon hit cooldowns. */
export interface Foe {
  kind: EnemyKind; hp: number; max: number; kb: number; flash: number; hit: Record<string, number>;
  burn: number; burnDps: number; slow: number; frozen: number;
}
/** A recycled ground/scenery slot: its index, the cell it shows ("gx,gy") and the grid width it was made for. */
export interface Slot { i: number; cell: string; cols: number }

export interface Candle {
  actions: "pick1" | "pick2" | "pick3" | "pause" | "restart" | "showcase";
  axes: "moveX" | "moveY";
  sounds: "bolt" | "whip" | "zap" | "pool" | "hit" | "kill" | "gem" | "coin" | "levelup" | "chest" | "evolve"
    | "hurt" | "heal" | "bell" | "boss" | "select" | "death" | "theme";
  images: "ground" | "logo";
  /** Each tag and the `data` its entities keep (`{}`: none). */
  tags: {
    hero: {}; elite: {}; boss: {}; fx: {}; label: {};
    enemy: Foe;
    shot: WeaponHit & { pierce: number; life: number; vel: Vec2; turn: number };
    orb: WeaponHit & { weapon: WeaponId; index: number; count: number; radius: number; until: number };
    pool: WeaponHit & { pierce: number; life: number; radius: number; tick: number; follow: boolean };
    slash: { life: number }; zap: { life: number };
    gem: { v: number; pull: number };
    item: { kind: ItemKind; pull: number };
    brazier: { hp: number };
    ground: Slot; prop: Slot;
    /** `seg`: the segment index; -1 = the dark plate under them. */
    hpbar: { seg: number };
  };
  sheets: {
    wick: "idle" | "walk"; taper: "idle" | "walk"; lumen: "idle" | "walk"; ash: "idle" | "walk"; vesper: "idle" | "walk"; tallow: "idle" | "walk";
    moth: "fly"; shade: "fly"; ghoul: "fly"; wisp: "fly"; gargoyle: "fly"; queen: "fly"; snuffer: "fly";
    items: "xp1" | "xp2" | "xp3" | "coin" | "cake" | "magnet" | "bell" | "wax";
    props: "chest" | "open" | "brazier" | "grave" | "reeds" | "shrooms" | "stones" | "stump";
    fx: "bolt" | "lance" | "slash" | "lantern" | "pool" | "spark";
  };
  layers: "floor" | "fx";
  locales: "pt" | "en";
  fonts: "pixel" | "icons" | "bigicons";
  categories: "hero" | "enemy" | "shot";
  ui: {
    title: "gold" | "play" | "heroes" | "shop" | "hero" | "best" | "lang";
    heroes: "back" | `h${HeroSlot}${"" | "name" | "desc" | "cost"}`;
    shop: "gold" | "back" | ShopId;
    hud: "xp" | "lv" | "time" | "kills" | "gold" | "weapons" | "passives" | "banner";
    levelup: `c${1 | 2 | 3}` | `c${1 | 2 | 3}${Card}`;
    chest: "icon" | "title" | "name" | "desc" | "ok";
    paused: "resume" | "quit" | "build";
    over: "title" | "time" | "level" | "kills" | "gold" | "best" | "again" | "menu";
  };
  scenes: { title: undefined; play: { hero: HeroId; demo?: boolean }; over: RunResult };
}

export type Tag = TagOf<Candle>;
export type Spec<T extends Tag> = EntitySpec<Candle, T>;
export type Ent<T extends Tag = never> = Entity<Candle, T>;
export type Ctx<S> = Context<Candle, S>;
export type Color = `#${string}`;
