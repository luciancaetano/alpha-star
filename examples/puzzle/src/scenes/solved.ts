import { scene } from "../alpha";
import { LEVELS } from "../levels";

// Records the best move count and the next level to start from (ctx.save survives restarts).
export default scene("solved", {
  enter(ctx, { level, moves }) {
    const key = `best${level}`;
    const best = ctx.save.get(key, 0);
    if (!best || moves < best) ctx.save.set(key, moves);
    const next = (level + 1) % LEVELS.length;
    ctx.save.set("level", next);
    ctx.ui.set("solved", "moves", ctx.t("solved.moves", { count: moves }));
    ctx.ui.on("solved", "next", "click", () => ctx.go("play", { level: next }));
    ctx.ui.show("solved");
  },
});
