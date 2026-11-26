// Type test for ctx.ui.toggle / attr and their simulate expectations.
import { define } from "alpha";
import { simulate } from "alpha/test";

interface G {
  actions: "go"; sounds: never; images: never; tags: { hero: {} };
  ui: { hud: "hp" | "xp" | "icon" };
  scenes: { play: undefined };
}
export const { scene, game } = define<G>();
const play = scene("play", {
  update(ctx) {
    ctx.ui.toggle("hud", "hp", "low", true);
    ctx.ui.attr("hud", "xp", "value", 0.5);
    ctx.ui.attr("hud", "icon", "src", "img/bolt.png");
    ctx.ui.attr("hud", "hp", "disabled", false);
    ctx.ui.attr("hud", "hp", "title", null);
    // @ts-expect-error `on` is required: toggling has no hidden state
    ctx.ui.toggle("hud", "hp", "low");
    // @ts-expect-error unknown element id
    ctx.ui.toggle("hud", "mp", "low", true);
    // @ts-expect-error attribute values are text, numbers, booleans or null
    ctx.ui.attr("hud", "xp", "value", { v: 1 });
  },
});
const g = game({ config: { title: "t", size: [10, 10], seed: 1, actions: { go: ["Space"] } }, start: "play", scenes: { play }, assets: { sounds: {} } });
export default g;
export function checks() {
  const run = simulate(g, { seconds: 1 });
  run.expect.classed("hud", "hp", "low").equals(true);
  run.expect.attr("hud", "xp", "value").equals("0.5");
  // @ts-expect-error unknown document
  run.expect.attr("menu", "xp", "value");
}
