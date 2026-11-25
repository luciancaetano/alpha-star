// Bench (e): an RmlUi document with 200 elements whose text changes every frame.
import { define } from "alpha";
const { scene, game } = define();

const play = scene("play", {
  enter(ctx) {
    ctx.ui.show("hud");
  },
  update(ctx) {
    const t = Math.floor(ctx.time * 60);
    for (let i = 0; i < 200; i++) ctx.ui.set("hud", "e" + i, (t + i) % 1000);
  },
});

export default game({
  config: { title: "bench-ui", size: [640, 360], seed: 1, actions: {} },
  assets: { ui: { hud: "hud.rml" } },
  start: "play",
  scenes: { play },
});
