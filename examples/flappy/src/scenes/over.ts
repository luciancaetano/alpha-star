import { scene } from "../alpha";
import { centered } from "../ui";

export default scene("over", {
  enter(ctx, { score }) {
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    centered(ctx.ui, "GAME OVER", 180, 32);
    centered(ctx.ui, `score ${score}   best ${best}`, 230, 18);
    centered(ctx.ui, "press flap", 300, 16);
  },
  update(ctx) {
    if (ctx.input.pressed("flap") || ctx.input.pressed("restart")) ctx.go("play");
  },
});
