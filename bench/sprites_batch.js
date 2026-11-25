// Bench (a, batch): the same 10k sprites moved with ctx.positions (one read, one write per frame).
import { define } from "alpha";
const { scene, game } = define();

const COUNT = 10000;
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
    const p = ctx.positions("s");
    for (let i = 0; i < p.count; i++) {
      p.x[i] = (p.x[i] + 1) % 640;
      p.y[i] = (p.y[i] + 0.5) % 360;
    }
    p.write();
  },
});

export default game({
  config: { title: "bench-sprites-batch", size: [640, 360], seed: 1, actions: {} },
  assets: { images: { dot: "dot.bmp" } },
  start: "play",
  scenes: { play },
});
