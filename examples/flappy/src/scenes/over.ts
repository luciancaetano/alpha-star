import { scene } from "../alpha";
import { centered } from "../ui";

export default scene("over", {
  enter(ctx, { score }) {
    ctx.audio.tone({ wave: "noise", freq: 1, duration: 0.3, volume: 0.3, release: 0.25 });
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    centered(ctx.ui, "GAME OVER", 180, 28);
    centered(ctx.ui, `score ${score}  best ${best}`, 230, 14);
    centered(ctx.ui, "press flap", 300, 16);
  },
  update(ctx) {
    if (ctx.input.pressed("flap") || ctx.input.pressed("restart")) ctx.go("play");
  },
});
