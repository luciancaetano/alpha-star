// Level-up choices and treasure chests: pure rules over RunState (the random source is passed in).
import type { Random } from "alpha";
import { PASSIVE_IDS, PASSIVES, WEAPON_IDS, WEAPONS } from "../data/items";
import { SLOTS } from "../tuning";
import type { WeaponId } from "../types";
import type { Offer, OwnedWeapon, RunState } from "./run";

const owned = (s: RunState, id: WeaponId) => s.weapons.find((w) => w.id === id);

function candidates(s: RunState): Offer[] {
  const out: Offer[] = [];
  for (const id of WEAPON_IDS) {
    const w = owned(s, id);
    if (w ? !w.evo && w.level < SLOTS.weaponMax : s.weapons.length < SLOTS.weapons) out.push({ kind: "weapon", id });
  }
  const held = Object.keys(s.passives).length;
  for (const id of PASSIVE_IDS) {
    const lv = s.passives[id] ?? 0;
    if (lv ? lv < SLOTS.passiveMax : held < SLOTS.passives) out.push({ kind: "passive", id });
  }
  return out;
}

const isOwned = (s: RunState, o: Offer) =>
  o.kind === "weapon" ? !!owned(s, o.id) : o.kind === "passive" ? !!s.passives[o.id] : false;

/** Up to three distinct choices; owned items weigh double. Nothing left: gold and cake. */
export function rollOffers(s: RunState, random: Random): Offer[] {
  const pool = candidates(s).flatMap((o) => (isOwned(s, o) ? [o, o] : [o]));
  const picked: Offer[] = [];
  while (picked.length < 3 && pool.length) {
    const o = random.pick(pool);
    picked.push(o);
    for (let i = pool.length - 1; i >= 0; i--) if (pool[i] === o) pool.splice(i, 1);
  }
  return picked.length ? picked : [{ kind: "gold" }, { kind: "cake" }];
}

export function applyOffer(s: RunState, o: Offer, maxHp: number) {
  if (o.kind === "weapon") {
    const w = owned(s, o.id);
    if (w) {
      w.level += 1;
      w.cd = 0;
    } else s.weapons.push({ id: o.id, level: 1, evo: false, cd: 0.2 });
  } else if (o.kind === "passive") s.passives[o.id] = (s.passives[o.id] ?? 0) + 1;
  else if (o.kind === "gold") s.gold += 25;
  else s.hp = Math.min(maxHp, s.hp + 30);
}

export interface Card { icon: string; name: string; tag: string; desc: string }
/** ctx.t: texts come from assets/locales. */
export type T = (key: string, params?: Record<string, string | number>) => string;

export function describe(s: RunState, o: Offer, t: T): Card {
  if (o.kind === "gold") return { icon: "$", name: t("loot.purse"), tag: "", desc: t("loot.purseDesc") };
  if (o.kind === "cake") return { icon: "%", name: t("loot.cake"), tag: "", desc: t("loot.cakeDesc") };
  if (o.kind === "passive") {
    const lv = s.passives[o.id] ?? 0;
    const name = t(`passive.${o.id}.name`);
    return { icon: PASSIVES[o.id].icon, name, tag: lv ? t("levelup.lv", { n: lv + 1 }) : t("levelup.new"), desc: t(`passive.${o.id}.desc`) };
  }
  const w = owned(s, o.id);
  const tag = w ? t("levelup.lv", { n: w.level + 1 }) : t("levelup.new");
  return { icon: WEAPONS[o.id].icon, name: t(`weapon.${o.id}.name`), tag, desc: t(`weapon.${o.id}.l${w ? w.level + 1 : 1}`) };
}

/** A weapon ready to evolve: max level, its passive held, not evolved yet. */
export const evolvable = (s: RunState): OwnedWeapon | undefined =>
  s.weapons.find((w) => !w.evo && w.level >= SLOTS.weaponMax && (s.passives[WEAPONS[w.id].needs] ?? 0) > 0);

/** Opens a chest: evolves a ready weapon, else levels an owned item, else gold. */
export function openChest(s: RunState, random: Random, t: T): Card {
  const evo = evolvable(s);
  if (evo) {
    evo.evo = true;
    evo.cd = 0;
    return { icon: WEAPONS[evo.id].evoIcon, name: t(`weapon.${evo.id}.evo`), tag: t("chest.evolution"), desc: t(`weapon.${evo.id}.evoDesc`) };
  }
  const upgrades = candidates(s).filter((o) => isOwned(s, o));
  if (!upgrades.length) {
    s.gold += 60;
    return { icon: "$", name: t("loot.hoard"), tag: t("chest.treasure"), desc: t("loot.hoardDesc") };
  }
  const o = random.pick(upgrades);
  const card = describe(s, o, t);
  applyOffer(s, o, 0);
  return { ...card, tag: t("chest.treasure") };
}
