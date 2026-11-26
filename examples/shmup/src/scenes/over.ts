import { scene } from "../alpha";

// The music fades out here and starts again with the next round.
export default scene("over", {
  enter(ctx, { score }) {
    ctx.audio.music.stop(1.5);
    ctx.ui.set("over", "score", `SCORE ${score}`);
    ctx.ui.on("over", "again", "click", () => ctx.go("play"));
    ctx.ui.show("over");
  },
  update(ctx) {
    if (ctx.input.pressed("restart")) ctx.go("play");
  },
});
