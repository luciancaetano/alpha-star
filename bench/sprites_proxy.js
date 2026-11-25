// Bench (a, per-entity proxies): sprites moved through entity.pos.x / pos.y. 2000, not
// 10000: at 10000 one step exceeds the 250 ms call budget (measured; see docs/PERF.md).
import { define } from "alpha";
const { scene, game } = define();

const COUNT = 2000;
let spawned = 0;
// 1000 per step during warm-up: 10000 in one call exceeds the 250 ms call budget.
function spawnSome(ctx) {
  for (let i = 0; i < 1000 && spawned < COUNT; i++, spawned++) {
    ctx.spawn({ pos: [ctx.random.range(0, 640), ctx.random.range(0, 360)], tags: ["s"], shape: { image: "dot" } });
  }
}
const play = scene("play", {
  update(ctx) {
    spawnSome(ctx);
    for (const e of ctx.findAll("s")) {
      e.pos.x = (e.pos.x + 1) % 640;
      e.pos.y = (e.pos.y + 0.5) % 360;
    }
  },
});

export default game({
  config: { title: "bench-sprites-proxy", size: [640, 360], seed: 1, actions: {} },
  assets: { images: { dot: "dot.bmp" } },
  start: "play",
  scenes: { play },
});
