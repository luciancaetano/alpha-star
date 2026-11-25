import { vec } from "alpha";
import { scene } from "../alpha";
import { player } from "../prefabs/player";
import { PLAYER, VIEW } from "../tuning";

export default scene("play", {
  enter(ctx) {
    ctx.spawn(player([VIEW.width / 2, VIEW.height / 2]));
  },

  update(ctx) {
    const p = ctx.find("player");
    const move = vec.scale(vec.norm([ctx.input.axis("moveX"), ctx.input.axis("moveY")]), PLAYER.speed * ctx.dt);
    p.pos.x = Math.max(0, Math.min(VIEW.width, p.pos.x + move[0]));
    p.pos.y = Math.max(0, Math.min(VIEW.height, p.pos.y + move[1]));
    if (ctx.input.pressed("jump")) {
      ctx.audio.play("blip");
      ctx.tween(p, { to: { scale: 1.4 }, seconds: PLAYER.hop, ease: "quadOut", yoyo: true });
    }
  },
});
