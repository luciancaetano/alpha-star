// Gems and items fly to the hero inside the magnet radius; braziers break into items.
import type { Vec2 } from "alpha";
import { vec } from "alpha";
import { burst, label, nova } from "../prefabs/effects";
import { brazier, item, type ItemKind } from "../prefabs/pickups";
import { BRAZIER, PICKUP } from "../tuning";
import type { Color, Ent } from "../types";
import { at, kill } from "./combat";
import { gainXp, type Play, type Stats } from "./run";

function pullTo(ctx: Play, e: Ent<"gem"> | Ent<"item">, hero: Ent<"hero">, radius: number): boolean {
  const d = vec.dist(at(e), at(hero));
  if (d < PICKUP.grab) return true;
  if (e.data.pull > 0 || d < radius) {
    e.data.pull = Math.min(PICKUP.pull * 1.5, e.data.pull + 600 * ctx.dt);
    const step = vec.scale(vec.norm(vec.sub(at(hero), at(e))), Math.max(e.data.pull, 60) * ctx.dt);
    e.pos.x += step[0];
    e.pos.y += step[1];
  }
  return false;
}

function collectGems(ctx: Play, hero: Ent<"hero">, st: Stats) {
  let got = 0;
  for (const g of ctx.findAll("gem")) {
    if (!pullTo(ctx, g, hero, st.magnet)) continue;
    got += g.data.v;
    ctx.destroy(g);
  }
  if (!got) return;
  ctx.audio.play("gem", { volume: 0.25, pitch: 1 + Math.min(0.6, ctx.state.xp / 80) });
  ctx.state.pending += gainXp(ctx.state, got * st.growth);
}

/** Floating label: locale key and color. */
const ITEM_TEXT: Record<Exclude<ItemKind, "chest">, [string, Color]> = {
  coin: ["loot.coin", "#ffd36b"], cake: ["loot.heal", "#ff8fa3"], magnet: ["loot.magnet", "#8fd3ff"], bell: ["loot.bell", "#fff2b0"],
};

function useItem(ctx: Play, hero: Ent<"hero">, kind: ItemKind, st: Stats) {
  if (kind === "chest") {
    ctx.state.chests += 1;
    ctx.audio.play("chest");
    return;
  }
  if (kind === "coin") ctx.state.gold += Math.round(ctx.random.int(3, 10) * st.greed);
  if (kind === "cake") ctx.state.hp = Math.min(st.maxHp, ctx.state.hp + PICKUP.cakeHeal);
  if (kind === "magnet") for (const g of ctx.findAll("gem")) g.data.pull = 120;
  if (kind === "bell") {
    for (const e of ctx.queryCircle(at(hero), PICKUP.bellRadius, { tags: ["enemy"] })) if (!e.has("boss")) kill(ctx, e);
    ctx.spawn(nova(at(hero), "#fff2b0"));
    ctx.camera.shake(4, 0.4);
  }
  ctx.audio.play(kind === "cake" ? "heal" : kind === "bell" ? "bell" : "coin");
  const [key, color] = ITEM_TEXT[kind];
  const text = ctx.t(key);
  const l = ctx.spawn(label([hero.pos.x, hero.pos.y - 18], text, color));
  ctx.tween(l, { to: { y: hero.pos.y - 34, alpha: 0 }, seconds: 0.9, onDone: () => ctx.destroy(l) });
}

function collectItems(ctx: Play, hero: Ent<"hero">, st: Stats) {
  for (const i of ctx.findAll("item")) {
    if (!pullTo(ctx, i, hero, i.data.kind === "chest" ? PICKUP.grab : st.magnet)) continue;
    ctx.destroy(i);
    useItem(ctx, hero, i.data.kind, st);
  }
}

const LOOT: Array<[number, ItemKind]> = [[0.4, "cake"], [0.72, "coin"], [0.9, "magnet"], [1, "bell"]];

/** A brazier in the dark ahead; any weapon breaks it. */
function braziers(ctx: Play, hero: Ent<"hero">) {
  ctx.state.brazierCd -= ctx.dt;
  if (ctx.state.brazierCd > 0 || ctx.findAll("brazier").length >= BRAZIER.max) return;
  ctx.state.brazierCd = BRAZIER.every;
  const [, , vw] = ctx.camera.view;
  ctx.spawn(brazier(vec.add(at(hero), vec.fromAngle(ctx.random.range(0, 360), vw * 0.45))));
}

export function breakBrazier(ctx: Play, b: Ent<"brazier">) {
  if (!b.alive) return;
  const pos: Vec2 = at(b);
  const roll = ctx.random.range(0, 1);
  const kind = LOOT.find(([p]) => roll < p)?.[1] ?? "coin";
  ctx.destroy(b);
  ctx.spawn(burst(pos, "#ffa04a", 24, 90));
  ctx.spawn(item(pos, kind));
}

const BREAKERS: Array<[Parameters<Play["findAll"]>[0], number]> = [["shot", 12], ["orb", 14], ["pool", 20], ["slash", 34], ["zap", 18]];

/** Any weapon touching a brazier breaks it (none of these collide with it: plain distance checks). */
function hitBraziers(ctx: Play) {
  for (const b of ctx.findAll("brazier")) {
    const hit = BREAKERS.some(([tag, r]) => ctx.findAll(tag).some((s) => vec.dist(at(s), at(b)) < r));
    if (hit) breakBrazier(ctx, b);
  }
}

export function updatePickups(ctx: Play, hero: Ent<"hero">, st: Stats) {
  collectGems(ctx, hero, st);
  collectItems(ctx, hero, st);
  braziers(ctx, hero);
  hitBraziers(ctx);
}
