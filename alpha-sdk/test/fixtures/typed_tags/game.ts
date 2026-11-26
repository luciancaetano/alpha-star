// Type test for tag data (GameTypes.tags): `typecheck` must report no problem.
// Each `@ts-expect-error` line must BE an error (unused ones are errors themselves).
import { define, type Entity, type EntitySpec, type TagOf } from "alpha";

type Kind = "moth" | "ghoul";
interface G {
  actions: "go";
  sounds: never;
  images: never;
  tags: { hero: {}; enemy: { kind: Kind; hp: number }; boss: { phase: number }; gem: { v: number }; fx: {} };
  ui: {};
  scenes: { play: undefined };
}
type Ent<T extends TagOf<G> = never> = Entity<G, T>;

const enemy = (kind: Kind): EntitySpec<G, "enemy"> => ({ tags: ["enemy"], pos: [0, 0], data: { kind, hp: 3 } });
const boss = (): EntitySpec<G, "enemy" | "boss"> => ({ tags: ["enemy", "boss"], pos: [0, 0], data: { kind: "ghoul", hp: 90, phase: 1 } });
const fx = (): EntitySpec<G, "fx"> => ({ tags: ["fx"], pos: [0, 0] });  // no data needed
// @ts-expect-error data is required: enemy has required fields
const noData = (): EntitySpec<G, "enemy"> => ({ tags: ["enemy"], pos: [0, 0] });
// @ts-expect-error boss data is missing `phase`
const halfBoss = (): EntitySpec<G, "enemy" | "boss"> => ({ tags: ["enemy", "boss"], pos: [0, 0], data: { kind: "moth", hp: 1 } });
// @ts-expect-error wrong field type
const badHp = (): EntitySpec<G, "enemy"> => ({ tags: ["enemy"], pos: [0, 0], data: { kind: "moth", hp: "3" } });
// @ts-expect-error unknown tag
const badTag = (): EntitySpec<G, "enemy"> => ({ tags: ["enmy"], pos: [0, 0], data: { kind: "moth", hp: 3 } });

function hurt(e: Ent<"enemy">, dmg: number): boolean {
  e.data.hp -= dmg;  // a number: no cast
  const k: Kind = e.data.kind;
  // @ts-expect-error enemy data has no `v`
  e.data.v = 1;
  if (e.has("boss")) e.data.phase += 1;  // has() narrows to boss data
  // @ts-expect-error not narrowed outside the guard
  e.data.phase = 0;
  return k === "moth" && e.data.hp <= 0;
}

const anyEntity = (e: Ent) => e.id;  // every Entity<G, T> is an Entity<G>

export const { scene, game } = define<G>();

const play = scene("play", {
  enter(ctx) {
    const e = ctx.spawn(enemy("moth"));
    hurt(e, 1);
    anyEntity(e);
    const b = ctx.spawn(boss());
    b.data.phase = 2;
    hurt(b, 1);
    ctx.spawn(fx());
    ctx.spawn({ tags: ["gem"], pos: [1, 1], data: { v: 2 } }).data.v += 1;
    // @ts-expect-error gem data is required
    ctx.spawn({ tags: ["gem"], pos: [1, 1] });
    // @ts-expect-error a gem has no hp
    ctx.spawn({ tags: ["gem"], pos: [1, 1], data: { v: 2 } }).data.hp;
  },
  update(ctx) {
    for (const e of ctx.findAll("enemy")) hurt(e, 1);
    ctx.find("gem").data.v += 1;
    for (const e of ctx.queryCircle([0, 0], 10, { tags: ["enemy"] })) hurt(e, 1);
    for (const e of ctx.queryCircle([0, 0], 10, { tags: ["enemy", "gem"] })) {
      // either tag: narrow first
      if (e.has("gem")) e.data.v += 1;
    }
    for (const e of ctx.queryCircle([0, 0], 10)) anyEntity(e);
    const hit = ctx.raycast([0, 0], [9, 9], { tags: ["enemy"] });
    if (hit) hurt(hit.entity, 1);
    ctx.onHit("gem", "enemy", (g, e) => { g.data.v += e.data.hp; });
    ctx.onEnter("hero", "gem", (_h, g) => { g.data.v = 0; });
    // @ts-expect-error unknown tag in a handler
    ctx.onHit("gems", "enemy", () => {});
    ctx.frozen = !ctx.frozen;
    ctx.tween(ctx.find("hero"), { to: { x: 1 }, seconds: 1 });
    ctx.spawn({ tags: ["fx"], pos: [0, 0], parent: ctx.find("enemy") });
  },
});

export default game({
  config: { title: "t", size: [10, 10], seed: 1, actions: { go: ["Space"] } },
  start: "play",
  scenes: { play },
  assets: { sounds: {} },
});
void noData; void halfBoss; void badHp; void badTag;
// Variance: an entity with fewer tags is not one with more.
declare const gemEnt: Ent<"gem">;
declare const both: Ent<"enemy" | "boss">;
// @ts-expect-error a gem is not an enemy
hurt(gemEnt, 1);
hurt(both, 1);  // more tags is fine
