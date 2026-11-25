import type { Context } from "alpha";
import { scene } from "../alpha";
import { ground } from "../prefabs/ground";
import { background } from "../prefabs/background";
import type { Flappy } from "../types";

type Settings = Context<Flappy, undefined>;
type Bus = "music" | "sfx";

const STEP = 0.1;

// Volumes in steps of 10%, rounded so repeated steps never drift.
function nudge(ctx: Settings, bus: Bus, by: number) {
  const v = Math.round((ctx.audio.bus(bus).volume + by) * 10) / 10;
  ctx.audio.bus(bus).volume = Math.min(1, Math.max(0, v));
  show(ctx, bus);
}

function show(ctx: Settings, bus: Bus) {
  ctx.ui.set("settings", bus, `${Math.round(ctx.audio.bus(bus).volume * 100)}%`);
}

// Every setting is persisted by the engine (settings.json) and applied at boot.
export default scene("settings", {
  enter(ctx) {
    ctx.spawn(background());
    ctx.spawn(ground());
    ctx.ui.show("settings");
    show(ctx, "music");
    show(ctx, "sfx");
    for (const bus of ["music", "sfx"] as const) {
      ctx.ui.on("settings", `${bus}-down`, "click", () => nudge(ctx, bus, -STEP));
      ctx.ui.on("settings", `${bus}-up`, "click", () => {
        nudge(ctx, bus, STEP);
        if (bus === "sfx") ctx.audio.play("point");  // hear the new level
      });
    }
    // Cycles through assets.locales; data-t elements re-translate themselves.
    ctx.ui.on("settings", "language", "click", () => {
      const all = ["en", "pt"] as const;
      ctx.locale = all[(all.indexOf(ctx.locale ?? "en") + 1) % all.length];
    });
    ctx.ui.on("settings", "back", "click", () => ctx.go("title"));
    ctx.ui.on("settings", "back", "back", () => ctx.go("title"));  // Escape / gamepad B
  },
});
