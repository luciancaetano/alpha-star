import { define } from "alpha";

interface T {
  actions: "jump";
  sounds: never;
  images: never;
  ui: {};
  tags: { hero: {} };
  scenes: { play: undefined };
}
const { scene, game } = define<T>();

const play = scene("play", {
  update(ctx) {
    if (ctx.input.pressed("fly")) ctx.go("play");
  },
});

export default game({
  config: { title: "t", size: [100, 100], seed: 1, actions: { jump: ["Space"] } },
  start: "play",
  scenes: { play },
  assets: { sounds: {} },
});
