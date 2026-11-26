import { define } from "alpha";

interface T {
  actions: "jump";
  axes: "moveX" | "throttle";
  sounds: never;
  images: never;
  ui: {};
  tags: { hero: {} };
  scenes: { play: undefined };
}
const { scene, game } = define<T>();

const play = scene("play", {
  update(ctx) {
    const x = ctx.input.axis("moveX") + ctx.input.player(2).axis("throttle");
    if (ctx.input.player(ctx.input.players() > 1 ? 2 : 1).pressed("jump") && x > 0) ctx.input.rumble(0.3, 0.1, 1);
  },
});

export default game({
  config: {
    title: "t", size: [100, 100], seed: 1, deadzone: 0.25,
    actions: { jump: ["Space", "GamepadA", "GamepadLeftY-"] },
    axes: { moveX: ["GamepadLeftX", "A/D"], throttle: ["GamepadRT"] },
  },
  start: "play",
  scenes: { play },
  assets: { sounds: {} },
});
