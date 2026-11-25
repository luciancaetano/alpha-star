import { scene } from "../alpha";

export default scene("over", {
  enter(ctx, { score }) {
    ctx.audio.tone({ wave: "noise", freq: 1, duration: 0.3, volume: 0.3, release: 0.25 });
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    ctx.ui.set("over", "score", score);
    ctx.ui.set("over", "best", best);
    ctx.ui.show("over");
    // Mouse, touch, Enter/Space or gamepad A on the focused button.
    ctx.ui.on("over", "again", "click", () => ctx.go("play"));
  },
  update(ctx) {
    if (ctx.input.pressed("flap") || ctx.input.pressed("restart")) ctx.go("play");
  },
});
