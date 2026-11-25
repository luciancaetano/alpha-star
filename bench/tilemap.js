// Bench (d): a 500x500 tile map (8 px tiles) with the camera scrolling across it.
import { define } from "alpha";
const { scene, game } = define();

const play = scene("play", {
  enter(ctx) {
    ctx.spawnMap("big");
  },
  update(ctx) {
    ctx.camera.pos.x = (ctx.camera.pos.x + 3) % 3600;
    ctx.camera.pos.y = (ctx.camera.pos.y + 2) % 3800;
  },
});

export default game({
  config: { title: "bench-tilemap", size: [640, 360], seed: 1, actions: {} },
  assets: { maps: { big: "big.tmj" } },
  start: "play",
  scenes: { play },
});
