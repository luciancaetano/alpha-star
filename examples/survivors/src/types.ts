// Every name the game uses. The compiler rejects anything not listed here.
import type { Context, Entity, EntitySpec } from "alpha";

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

export interface Candle {
  actions: "pick1" | "pick2" | "pick3" | "pause" | "restart";
  axes: "moveX" | "moveY";
  sounds: "bolt" | "whip" | "zap" | "pool" | "hit" | "kill" | "gem" | "coin" | "levelup" | "chest" | "evolve"
    | "hurt" | "heal" | "bell" | "boss" | "select" | "death" | "theme";
  images: "ground" | "logo";
  tags: "hero" | "enemy" | "elite" | "boss" | "shot" | "orb" | "pool" | "slash" | "zap" | "gem" | "item" | "brazier"
    | "prop" | "ground" | "fx" | "label" | "hpbar";
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
  scenes: { title: undefined; play: { hero: HeroId }; over: RunResult };
}

export type Spec = EntitySpec<Candle["tags"], Candle["images"], Candle["sheets"], Candle["layers"], Candle["fonts"], Candle["categories"]>;
export type Ent = Entity<Candle["tags"], any>;
export type Ctx<S> = Context<Candle, S>;
export type Color = `#${string}`;
