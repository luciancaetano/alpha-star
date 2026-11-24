import { define } from "alpha";
import { SPEED, clamp } from "./helpers";

interface T {
  actions: "jump";
  sounds: never;
  tags: "hero";
  scenes: { play: undefined; over: { score: number } };
}
const { scene, game } = define<T>();

const play = scene("play", {
  state: () => ({ jumps: 0 }),
  update(ctx) {
    if (ctx.input.pressed("jump")) ctx.state.jumps += 1;
    if (ctx.state.jumps > 3) ctx.go("over", { score: clamp(SPEED, 0, 10) });
  },
});
const over = scene("over", { enter(_ctx, { score }) { void score; } });

export default game({
  config: { title: "t", size: [100, 100], seed: 1, actions: { jump: ["Space"] } },
  start: "play",
  scenes: { play, over },
  assets: { sounds: {} },
});
