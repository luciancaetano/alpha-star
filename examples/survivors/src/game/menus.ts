// HUD texts and the in-run menus (level up, treasure, pause). Icons are glyphs of the icon fonts;
// words come from assets/locales through ctx.t.
import { PASSIVES, WEAPONS } from "../data/items";
import { nova } from "../prefabs/effects";
import { SLOTS, xpFor } from "../tuning";
import type { Ent, PassiveId } from "../types";
import { at } from "./combat";
import { applyOffer, type Card, describe, openChest, rollOffers } from "./offers";
import { clock, type Play, stats } from "./run";

const XP_SEGMENTS = 40;

export function hud(ctx: Play) {
  const s = ctx.state;
  const filled = Math.min(XP_SEGMENTS, Math.floor((s.xp / xpFor(s.level)) * XP_SEGMENTS));
  ctx.ui.set("hud", "xp", "#".repeat(filled) + "-".repeat(XP_SEGMENTS - filled));
  ctx.ui.set("hud", "lv", ctx.t("hud.lv", { n: s.level }));
  ctx.ui.set("hud", "time", clock(s.time));
  ctx.ui.set("hud", "kills", ctx.t("hud.foes", { n: s.kills }));
  ctx.ui.set("hud", "gold", ctx.t("hud.gold", { n: s.gold }));
  const weapons = s.weapons.map((w) => (w.evo ? WEAPONS[w.id].evoIcon : WEAPONS[w.id].icon)).join("");
  ctx.ui.set("hud", "weapons", weapons + "*".repeat(Math.max(0, SLOTS.weapons - s.weapons.length)));
  const passives = (Object.keys(s.passives) as PassiveId[]).map((p) => PASSIVES[p].icon).join("");
  ctx.ui.set("hud", "passives", passives + "*".repeat(Math.max(0, SLOTS.passives - [...passives].length)));
}

function showCard(ctx: Play, n: 1 | 2 | 3, card: Card | undefined) {
  ctx.ui.set("levelup", `c${n}icon`, card?.icon ?? "");
  ctx.ui.set("levelup", `c${n}name`, card?.name ?? "");
  ctx.ui.set("levelup", `c${n}tag`, card?.tag ?? "");
  ctx.ui.set("levelup", `c${n}desc`, card?.desc ?? "");
}

function treasure(ctx: Play, hero: Ent<"hero">) {
  const s = ctx.state;
  s.chests -= 1;
  const card = openChest(s, ctx.random, (k, p) => ctx.t(k, p));
  ctx.ui.set("chest", "icon", card.icon);
  ctx.ui.set("chest", "title", card.tag);
  ctx.ui.set("chest", "name", card.name);
  ctx.ui.set("chest", "desc", card.desc);
  ctx.ui.show("chest");
  ctx.audio.play(card.tag === ctx.t("chest.evolution") ? "evolve" : "levelup");
  ctx.spawn(nova(at(hero), "#ffd36b"));
  s.mode = "chest";
}

/** Opens the next pending menu (chests first); returns false when none is due. */
export function nextMenu(ctx: Play, hero: Ent<"hero">): boolean {
  const s = ctx.state;
  if (s.chests > 0) {
    treasure(ctx, hero);
    return true;
  }
  if (s.pending <= 0) return false;
  s.pending -= 1;
  s.offers = rollOffers(s, ctx.random);
  ([1, 2, 3] as const).forEach((n) => showCard(ctx, n, s.offers[n - 1] && describe(s, s.offers[n - 1], (k, p) => ctx.t(k, p))));
  ctx.ui.show("levelup");
  ctx.audio.play("levelup");
  ctx.spawn(nova(at(hero), "#8fd3ff"));
  s.mode = "levelup";
  return true;
}

export function choose(ctx: Play, n: number) {
  const s = ctx.state;
  const offer = s.offers[n];
  if (s.mode !== "levelup" || !offer) return;
  applyOffer(s, offer, stats(s).maxHp);
  ctx.audio.play("select");
  closeMenus(ctx);
}

export function closeMenus(ctx: Play) {
  ctx.ui.hide("levelup");
  ctx.ui.hide("chest");
  ctx.ui.hide("paused");
  ctx.state.mode = "play";
}

export function pause(ctx: Play) {
  const s = ctx.state;
  const lines = [
    ...s.weapons.map((w) => `${ctx.t(`weapon.${w.id}.${w.evo ? "evo" : "name"}`)}  ${w.evo ? ctx.t("paused.max") : ctx.t("hud.lv", { n: w.level })}`),
    ...(Object.keys(s.passives) as PassiveId[]).map((p) => `${ctx.t(`passive.${p}.name`)}  ${ctx.t("hud.lv", { n: s.passives[p] ?? 0 })}`),
  ];
  ctx.ui.set("paused", "build", lines.join("\n"));
  ctx.ui.show("paused");
  s.mode = "paused";
}

/** Menu buttons: wired once per scene enter (handlers run at the start of the next step). */
export function wireMenus(ctx: Play, quit: () => void) {
  ([1, 2, 3] as const).forEach((n) => ctx.ui.on("levelup", `c${n}`, "click", () => choose(ctx, n - 1)));
  ctx.ui.on("chest", "ok", "click", () => closeMenus(ctx));
  ctx.ui.on("paused", "resume", "click", () => closeMenus(ctx));
  ctx.ui.on("paused", "resume", "back", () => closeMenus(ctx));
  ctx.ui.on("paused", "quit", "click", quit);
}

/** Night events (game/enemies.ts) announce themselves: a banner for 3.5 s; bosses roar and shake. */
export function wireBanners(ctx: Play) {
  ctx.on("nightEvent", ({ at: when, boss }) => {
    ctx.ui.set("hud", "banner", ctx.t(`event.${when}`));
    ctx.after(3.5, () => ctx.ui.set("hud", "banner", ""));
    if (!boss) return;
    ctx.audio.play("boss");
    ctx.camera.shake(3, 0.6);
  });
}
