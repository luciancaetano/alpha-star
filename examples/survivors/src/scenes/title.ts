// Title: play, pick a hero, spend gold on permanent upgrades (kept in the save).
import type { Context } from "alpha";
import { scene } from "../alpha";
import { clock } from "../game/run";
import { HEROES } from "../tuning";
import type { Candle, HeroId, HeroSlot, ShopId } from "../types";

type Title = Context<Candle, { hero: HeroId }>;
type Shop = Partial<Record<ShopId, number>>;

/** Names live in assets/locales (upgrade.<id>). */
export const SHOP: Record<ShopId, { max: number; base: number }> = {
  might: { max: 5, base: 60 }, armor: { max: 3, base: 90 }, vigor: { max: 5, base: 50 }, haste: { max: 5, base: 70 },
  swift: { max: 3, base: 80 }, magnet: { max: 3, base: 40 }, growth: { max: 5, base: 70 }, greed: { max: 5, base: 50 },
};
const SHOP_IDS = Object.keys(SHOP) as ShopId[];
const HERO_IDS = Object.keys(HEROES) as HeroId[];

export const shopCost = (id: ShopId, lv: number) => SHOP[id].base * (lv + 1);

function refreshShop(ctx: Title, gold: number) {
  const shop = ctx.save.get<Shop>("shop", {});
  ctx.ui.set("shop", "gold", ctx.t("title.gold", { n: gold }));
  for (const id of SHOP_IDS) {
    const lv = shop[id] ?? 0;
    const price = lv >= SHOP[id].max ? ctx.t("shop.max") : `${shopCost(id, lv)}`;
    ctx.ui.set("shop", id, `${ctx.t(`upgrade.${id}`)}  ${lv}/${SHOP[id].max}  ${price}`);
  }
}

function refreshHeroes(ctx: Title, gold: number) {
  const owned = ctx.save.get<HeroId[]>("heroes", ["wick"]);
  HERO_IDS.forEach((id, i) => {
    const n = (i + 1) as HeroSlot;
    ctx.ui.set("heroes", `h${n}name`, ctx.t(`hero.${id}.name`));
    ctx.ui.set("heroes", `h${n}desc`, ctx.t(`hero.${id}.desc`));
    const cost = ctx.state.hero === id ? ctx.t("heroes.picked") : owned.includes(id) ? "" : ctx.t("heroes.cost", { n: HEROES[id].cost });
    ctx.ui.set("heroes", `h${n}cost`, cost);
  });
  ctx.ui.set("title", "gold", ctx.t("title.gold", { n: gold }));
}

function refresh(ctx: Title) {
  const gold = ctx.save.get("gold", 0);
  ctx.ui.set("title", "hero", ctx.t(`hero.${ctx.state.hero}.name`));
  const best = ctx.save.get("best", 0);
  ctx.ui.set("title", "best", best ? ctx.t("title.best", { time: clock(best) }) : ctx.t("title.goal"));
  refreshShop(ctx, gold);
  refreshHeroes(ctx, gold);
}

/** pt is config.locale (the default); the choice is kept in the player's settings by the engine. */
function toggleLanguage(ctx: Title) {
  ctx.locale = ctx.locale === "pt" ? "en" : "pt";
  ctx.audio.play("select");
  refresh(ctx);
}

function buy(ctx: Title, id: ShopId) {
  const shop = ctx.save.get<Shop>("shop", {});
  const lv = shop[id] ?? 0;
  const gold = ctx.save.get("gold", 0);
  if (lv >= SHOP[id].max || gold < shopCost(id, lv)) return void ctx.audio.play("hurt", { volume: 0.4 });
  ctx.save.set("gold", gold - shopCost(id, lv));
  ctx.save.set("shop", { ...shop, [id]: lv + 1 });
  ctx.audio.play("coin");
  refresh(ctx);
}

function pickHero(ctx: Title, id: HeroId) {
  const owned = ctx.save.get<HeroId[]>("heroes", ["wick"]);
  const gold = ctx.save.get("gold", 0);
  if (!owned.includes(id)) {
    if (gold < HEROES[id].cost) return void ctx.audio.play("hurt", { volume: 0.4 });
    ctx.save.set("gold", gold - HEROES[id].cost);
    ctx.save.set("heroes", [...owned, id]);
  }
  ctx.state.hero = id;
  ctx.save.set("hero", id);
  ctx.audio.play("select");
  refresh(ctx);
}

function open(ctx: Title, doc: "title" | "heroes" | "shop") {
  for (const d of ["title", "heroes", "shop"] as const) ctx.ui.hide(d);
  ctx.ui.show(doc);
}

export default scene("title", {
  state: () => ({ hero: "wick" as HeroId }),
  enter(ctx) {
    ctx.state.hero = ctx.save.get<HeroId>("hero", "wick");
    ctx.ui.on("title", "lang", "click", () => toggleLanguage(ctx));
    ctx.ui.on("title", "play", "click", () => ctx.go("play", { hero: ctx.state.hero }));
    ctx.ui.on("title", "heroes", "click", () => open(ctx, "heroes"));
    ctx.ui.on("title", "shop", "click", () => open(ctx, "shop"));
    for (const doc of ["heroes", "shop"] as const) {
      ctx.ui.on(doc, "back", "click", () => open(ctx, "title"));
      ctx.ui.on(doc, "back", "back", () => open(ctx, "title"));
    }
    HERO_IDS.forEach((id, i) => ctx.ui.on("heroes", `h${(i + 1) as HeroSlot}`, "click", () => pickHero(ctx, id)));
    for (const id of SHOP_IDS) ctx.ui.on("shop", id, "click", () => buy(ctx, id));
    refresh(ctx);
    open(ctx, "title");
    ctx.postfx.add("vignette", { strength: 0.6, radius: 0.5 });
  },
});
