import { scene } from "../alpha";
import { bird } from "../prefabs/bird";
import { ground } from "../prefabs/ground";
import { BIRD } from "../tuning";

export default scene("title", {
  enter(ctx) {
    ctx.spawn(ground());
    ctx.spawn(bird(false));
    ctx.ui.show("title");
  },
  update(ctx) {
    // Idle bob as a pure function of time: deterministic.
    ctx.find("bird").pos.y = BIRD.startY + Math.sin(ctx.time * 4) * 6;
    if (ctx.input.pressed("flap")) ctx.go("play");
  },
});
