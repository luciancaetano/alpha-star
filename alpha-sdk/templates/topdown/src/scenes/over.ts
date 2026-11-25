import { scene } from "../alpha";

// Shows the score and the best one, kept in the save (survives restarts).
export default scene("over", {
  enter(ctx, { score }) {
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    ctx.ui.set("over", "score", `SCORE ${score}`);
    ctx.ui.set("over", "best", `BEST ${best}`);
    ctx.ui.on("over", "again", "click", () => ctx.go("play"));
    ctx.ui.show("over");
  },
  update(ctx) {
    if (ctx.input.pressed("restart")) ctx.go("play");
  },
});
