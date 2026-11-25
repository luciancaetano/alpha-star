import { scene } from "../alpha";
import { bird } from "../prefabs/bird";
import { ground } from "../prefabs/ground";
import { pipePair } from "../prefabs/pipes";
import { BIRD, PIPE } from "../tuning";

export default scene("play", {
  state: () => ({ score: 0 }),

  enter(ctx) {
    ctx.spawn(ground());
    ctx.spawn(bird(true));
    ctx.ui.show("hud");

    ctx.onHit("bird", "solid", () => ctx.go("over", { score: ctx.state.score }));
    ctx.onEnter("bird", "goal", (_bird, goal) => {
      ctx.state.score += 1;
      ctx.ui.set("hud", "score", ctx.state.score);
      ctx.destroy(goal);
      ctx.audio.play("point");
    });
    ctx.every(PIPE.every, () => pipePair(ctx.random).forEach((p) => ctx.spawn(p)));
  },

  update(ctx) {
    const b = ctx.find("bird");
    if (ctx.input.pressed("flap")) {
      b.body.velocity = [0, BIRD.flap];
      ctx.audio.play("flap");
    }
    b.rotation = Math.min(BIRD.maxTilt, Math.max(-30, b.body.velocity.y * 0.15));
    if (b.pos.y < 0) ctx.go("over", { score: ctx.state.score });

    for (const e of ctx.findAll("scrolls")) {
      if (e.pos.x < -PIPE.width) ctx.destroy(e);
    }
  },
});
