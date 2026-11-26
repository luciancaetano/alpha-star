// The night ends: banks the gold, keeps the best time.
import { scene } from "../alpha";
import { clock } from "../game/run";

export default scene("over", {
  enter(ctx, r) {
    const bank = ctx.save.get("gold", 0) + r.gold;
    ctx.save.set("gold", bank);
    const best = Math.max(r.seconds, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    ctx.ui.set("over", "title", ctx.t(r.dawn ? "over.dawn" : "over.dead"));
    ctx.ui.set("over", "time", ctx.t("over.time", { t: clock(r.seconds) }));
    ctx.ui.set("over", "level", ctx.t("over.level", { n: r.level }));
    ctx.ui.set("over", "kills", ctx.t("over.kills", { n: r.kills }));
    ctx.ui.set("over", "gold", ctx.t("over.gold", { n: r.gold, bank }));
    ctx.ui.set("over", "best", ctx.t("over.best", { t: clock(best) }));
    ctx.ui.on("over", "again", "click", () => ctx.go("play", { hero: r.hero }));
    ctx.ui.on("over", "menu", "click", () => ctx.go("title"));
    ctx.ui.show("over");
    ctx.postfx.add("vignette", { strength: 0.6, radius: 0.5 });
  },
  update(ctx) {
    if (ctx.input.pressed("restart")) ctx.go("title");
  },
});
