import { scene } from "../alpha";
import { bird } from "../prefabs/bird";
import { background } from "../prefabs/background";
import { ground } from "../prefabs/ground";
import { ART } from "../tuning";

export default scene("title", {
  enter(ctx) {
    ctx.spawn(background());
    ctx.spawn(ground());
    ctx.spawn(bird(false));
    ctx.ui.show("title");
    // Phones have no keyboard: the hints name the screen and the back button instead.
    if (ctx.platform.mobile) {
      ctx.ui.set("title", "hint", ctx.t("title.hintTouch"));
      ctx.ui.set("title", "settings", ctx.t("title.settingsTouch"));
    }
    // Music outlives scenes: start it once, it loops from then on.
    if (ctx.audio.musicPlaying !== "theme") ctx.audio.music("theme", { volume: 0.6, fadeIn: 1 });
  },
  update(ctx) {
    // Idle bob as a pure function of time: deterministic.
    ctx.find("bird").pos.y = ART.titleY + (ctx.settings.reducedMotion ? 0 : Math.sin(ctx.time * 3) * 6);
    if (ctx.input.pressed("settings")) ctx.go("settings");
    else if (ctx.input.pressed("flap")) ctx.go("play");
  },
});
