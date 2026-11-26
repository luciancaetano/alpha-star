// Bench (d): a 500x500 LDtk tile layer (8 px tiles) with the camera scrolling across it.
import { define } from "alpha";
const { scene, game } = define();

const play = scene("play", {
  enter(ctx) {
    ctx.spawnLevel("big", "Big", { background: false });
  },
  update(ctx) {
    ctx.camera.pos.x = (ctx.camera.pos.x + 3) % 3600;
    ctx.camera.pos.y = (ctx.camera.pos.y + 2) % 3800;
  },
});

export default game({
  config: { title: "bench-tilemap", size: [640, 360], seed: 1, actions: {} },
  assets: { worlds: { big: "big.ldtk" } },
  start: "play",
  scenes: { play },
});
