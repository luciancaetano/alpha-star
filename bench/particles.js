// Bench (c): 20k live particles (4 emitters of 5000).
import { define } from "alpha";
const { scene, game } = define();

const play = scene("play", {
  enter(ctx) {
    for (let i = 0; i < 4; i++) {
      ctx.spawn({
        pos: [80 + i * 160, 180],
        tags: [],
        emitter: { rate: 5000, max: 5000, lifetime: [1.5, 2], speed: [20, 80], angle: [0, 360], size: [3, 1],
          color: ["#ffcc66ff", "#ff660000"], shape: "rect" },
      });
    }
  },
});

export default game({
  config: { title: "bench-particles", size: [640, 360], seed: 1, actions: {} },
  start: "play",
  scenes: { play },
});
