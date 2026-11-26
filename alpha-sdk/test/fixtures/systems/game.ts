// Type test for systems, events and timers: `typecheck` must report no problem.
// Each `@ts-expect-error` line must BE an error (unused ones are errors themselves).
import { define, type Entity, type System } from "alpha";

interface G {
  actions: "go";
  sounds: never;
  images: never;
  tags: { hero: {}; enemy: { hp: number }; gem: { v: number } };
  events: { died: { enemy: Entity<G, "enemy">; by: string }; levelUp: { level: number }; dawn: undefined };
  ui: {};
  scenes: { play: undefined };
}
type State = { kills: number };

export const { scene, game } = define<G>();

// A system in its own module: typed by the game and the scene state.
const chase: System<G, State> = { name: "chase", tag: "enemy", each(ctx, e) { e.data.hp -= ctx.dt; } };
// @ts-expect-error an enemy has no `v`
const wrong: System<G, State> = { tag: "enemy", each(_ctx, e) { e.data.v = 1; } };
// @ts-expect-error unknown tag
const unknownTag: System<G, State> = { tag: "enemi", each() {} };
// @ts-expect-error unknown `when`
const badWhen: System<G, State> = { when: "paused", run() {} };

const play = scene("play", {
  state: () => ({ kills: 0 }),
  enter(ctx) {
    ctx.on("died", ({ enemy, by }) => { ctx.state.kills += enemy.data.hp <= 0 ? 1 : 0; void by; });
    ctx.on("levelUp", (p) => { const n: number = p.level; void n; });
    ctx.on("dawn", (p) => { const none: undefined = p; void none; });
    ctx.emit("dawn");
    ctx.emit("levelUp", { level: 2 });
    // @ts-expect-error payload required
    ctx.emit("levelUp");
    // @ts-expect-error wrong payload shape
    ctx.emit("levelUp", { lvl: 2 });
    // @ts-expect-error no payload for dawn
    ctx.emit("dawn", {});
    // @ts-expect-error unknown event
    ctx.on("dead", () => {});
    const hero = ctx.spawn({ tags: ["hero"], pos: [0, 0] });
    const t = ctx.every(1, () => {}, { owner: hero });
    if (t.active) t.cancel();
    ctx.after(0.5, () => {}).cancel();
  },
  systems: [
    chase,
    (ctx) => { ctx.state.kills += 0; },
    { name: "menu", when: "frozen", run(ctx) { ctx.frozen = false; } },
    { tag: "gem", each(_ctx, g) { g.data.v += 1; } },
    {
      tag: "enemy",
      each(ctx, e) {
        if (e.data.hp <= 0) {
          ctx.emit("died", { enemy: e, by: "time" });
          ctx.destroy(e);
        }
      },
    },
    // @ts-expect-error a gem system's entity is a gem
    { tag: "gem", each(_ctx, g) { g.data.hp = 1; } },
  ],
});

export default game({
  config: { title: "t", size: [10, 10], seed: 1, actions: { go: ["Space"] } },
  start: "play",
  scenes: { play },
  assets: { sounds: {} },
});
void wrong; void unknownTag; void badWhen;
