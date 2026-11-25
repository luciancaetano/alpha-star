import { scene } from "../alpha";
import { LEVELS } from "../levels";

// Continues where the player left off (the saved level).
export default scene("start", {
  enter(ctx) {
    ctx.go("play", { level: Math.min(ctx.save.get("level", 0), LEVELS.length - 1) });
  },
});
