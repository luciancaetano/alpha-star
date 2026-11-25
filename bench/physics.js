// Bench (b): 2k dynamic circle bodies falling into a box.
import { define } from "alpha";
const { scene, game } = define();

const COUNT = 2000;
const wall = (pos, size) => ({ pos, tags: [], body: { type: "static", collider: { rect: size } }, shape: { rect: size, color: "#888" } });
const play = scene("play", {
  enter(ctx) {
    ctx.spawn(wall([320, 355], [640, 10]));
    ctx.spawn(wall([5, 180], [10, 360]));
    ctx.spawn(wall([635, 180], [10, 360]));
    for (let i = 0; i < COUNT; i++) {
      const pos = [20 + (i % 100) * 6, 10 + Math.floor(i / 100) * 6];
      ctx.spawn({ pos, tags: ["ball"], body: { type: "dynamic", collider: { circle: 2.5 } }, shape: { circle: 2.5, color: "#4af" } });
    }
  },
});

export default game({
  config: { title: "bench-physics", size: [640, 360], seed: 1, gravity: [0, 400], actions: {} },
  start: "play",
  scenes: { play },
});
