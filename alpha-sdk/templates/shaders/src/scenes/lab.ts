import type { Context } from "alpha";
import { scene } from "../alpha";
import { adjust, cycle, label, level } from "../cycle";
import { floor, orb, orbitAt, title } from "../prefabs/scene";
import { INTENSITY, ORBS, SHADERS, VIEW } from "../tuning";
import type { ShaderLab } from "../types";

type State = { index: number; intensity: number };
type Lab = Context<ShaderLab, State>;

// One shader on at a time: remove the old one, add the new one, refresh the HUD.
function select(ctx: Lab, step: number) {
  ctx.postfx.remove(SHADERS[ctx.state.index]);
  ctx.state.index = cycle(ctx.state.index, step);
  ctx.postfx.set(SHADERS[ctx.state.index], { intensity: ctx.state.intensity, time: ctx.time });
  ctx.ui.set("hud", "name", label(ctx.state.index));
  ctx.audio.play("click");
}

function tweak(ctx: Lab, steps: number) {
  ctx.state.intensity = adjust(ctx.state.intensity, steps);
  ctx.ui.set("hud", "level", level(ctx.state.intensity));
  ctx.audio.play("click", { pitch: 0.5 + ctx.state.intensity });
}

// Phones have no keyboard: on-screen buttons do what the keys do.
function touchButtons(ctx: Lab) {
  ctx.ui.set("hud", "hint", "TAP THE BUTTONS");
  ctx.ui.on("touch", "prev", "click", () => select(ctx, -1));
  ctx.ui.on("touch", "next", "click", () => select(ctx, 1));
  ctx.ui.on("touch", "less", "click", () => tweak(ctx, -1));
  ctx.ui.on("touch", "more", "click", () => tweak(ctx, 1));
  ctx.ui.show("touch");
}

export default scene("lab", {
  state: (): State => ({ index: 0, intensity: INTENSITY.start }),
  enter(ctx) {
    floor(ctx.camera.view).forEach((tile) => ctx.spawn(tile));
    ctx.spawn(title());
    for (let i = 0; i < ORBS.count; i++) ctx.spawn(orb(i));
    ctx.postfx.set(SHADERS[ctx.state.index], { intensity: ctx.state.intensity, time: 0 });
    ctx.ui.set("hud", "name", label(ctx.state.index));
    ctx.ui.set("hud", "level", level(ctx.state.intensity));
    ctx.ui.show("hud");
    if (ctx.platform.mobile) touchButtons(ctx);
  },
  update(ctx) {
    if (ctx.input.pressed("next")) select(ctx, 1);
    if (ctx.input.pressed("prev")) select(ctx, -1);
    if (ctx.input.pressed("more")) tweak(ctx, 1);
    if (ctx.input.pressed("less")) tweak(ctx, -1);
  },

  // Run in this order after update, every step; a tag system runs once per living entity.
  systems: [
    { tag: "orb", each: (ctx, o) => { o.pos = orbitAt(o.data.i, ctx.time); } },
    // Animated uniform: ctx.time drives each shader's motion (render-only, never game state).
    { name: "uniforms", run: (ctx) => ctx.postfx.set(SHADERS[ctx.state.index], { intensity: ctx.state.intensity, time: ctx.time }) },
  ],
});
