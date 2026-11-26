// Bench (c2): 20k live smooth particles (pixelMode false; half circles, half rects).
import { define } from "alpha";
const { scene, game } = define();

const play = scene("play", {
  enter(ctx) {
    for (let i = 0; i < 4; i++) {
      ctx.spawn({
        pos: [80 + i * 160, 180],
        tags: [],
        emitter: { rate: 5000, max: 5000, lifetime: [1.5, 2], speed: [20, 80], angle: [0, 360], size: [3, 1],
          color: ["#ffcc66ff", "#ff660000"], shape: i % 2 ? "rect" : "circle" },
      });
    }
  },
});

export default game({
  config: { title: "bench-particles-smooth", size: [640, 360], seed: 1, actions: {}, pixelMode: false },
  start: "play",
  scenes: { play },
});
