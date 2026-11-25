// Entity specs: plain data built by functions.
import type { EntitySpec, Vec2 } from "alpha";
import { vec } from "alpha";
import { BULLET, ENEMY, HERO, VIEW, WALL } from "../tuning";
import type { Arena } from "../types";

type Spec = EntitySpec<Arena["tags"], never, Arena["sheets"]>;

// `data.player` says which player's input drives it; `aim` is the last direction moved.
export function hero(player: number, at: Vec2): Spec {
  return {
    tags: ["hero"],
    pos: at,
    z: 2,
    shape: { sheet: "hero", animation: "idle", ...(player === 2 ? { tint: "#9be7ff" } : {}) },
    body: { type: "dynamic", fixedRotation: true, collider: { circle: HERO.radius } },
    data: { player, hp: HERO.hp, bombs: HERO.bombs, aim: [1, 0], cooldown: 0 },
  };
}

export function enemy(at: Vec2): Spec {
  return {
    tags: ["enemy"],
    pos: at,
    z: 1,
    shape: { circle: ENEMY.radius, color: "#e74c3c", outline: "#7b241c" },
    body: { type: "dynamic", fixedRotation: true, collider: { circle: ENEMY.radius } },
  };
}

// A sensor: it passes through things and reports overlaps (ctx.onEnter).
export function bullet(from: Vec2, aim: Vec2): Spec {
  return {
    tags: ["bullet"],
    pos: vec.add(from, vec.scale(aim, HERO.radius + 4)),
    shape: { circle: BULLET.radius, color: "#ffe14d" },
    body: { type: "dynamic", bullet: true, velocity: vec.scale(aim, BULLET.speed), collider: { circle: BULLET.radius, sensor: true } },
    data: { life: BULLET.life },
  };
}

// The four walls around the arena.
export function walls(): Spec[] {
  const { width: w, height: h } = VIEW;
  const box = (pos: Vec2, size: Vec2): Spec => ({
    tags: ["wall"], pos, shape: { rect: size, color: "#34495e" }, body: { type: "static", collider: { rect: size } },
  });
  return [box([w / 2, WALL / 2], [w, WALL]), box([w / 2, h - WALL / 2], [w, WALL]),
          box([WALL / 2, h / 2], [WALL, h]), box([w - WALL / 2, h / 2], [WALL, h])];
}
