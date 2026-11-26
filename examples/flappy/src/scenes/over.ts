import { scene } from "../alpha";
import { ground } from "../prefabs/ground";
import { background } from "../prefabs/background";
import { FADE, WORLD } from "../tuning";

export default scene("over", {
  enter(ctx, { score }) {
    ctx.spawn(background(ctx.camera.view));
    ctx.audio.tone({ wave: "noise", freq: 1, duration: 0.3, volume: 0.3, release: 0.25 });
    const best = Math.max(score, ctx.save.get("best", 0));
    ctx.save.set("best", best);
    ctx.ui.set("over", "score", score);
    ctx.ui.set("over", "best", best);
    ctx.ui.show("over");
    // Mouse, touch, Enter/Space or gamepad A on the focused button.
    ctx.ui.on("over", "again", "click", () => ctx.go("play"));
    // The world fades to dark behind the game-over panel (update raises its alpha).
    ctx.spawn(ground(ctx.camera.view));
    ctx.spawn({
      tags: ["fade"], pos: [WORLD.width / 2, WORLD.height / 2], z: 10,
      shape: { rect: [WORLD.width, WORLD.height], color: "#000000", alpha: 0 },
    });
  },
  update(ctx) {
    const fade = ctx.find("fade");
    fade.alpha = Math.min(FADE.alpha, (ctx.time / FADE.seconds) * FADE.alpha);
    if (ctx.input.pressed("flap") || ctx.input.pressed("restart")) ctx.go("play");
  },
});
