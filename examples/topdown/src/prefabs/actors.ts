// Entity specs: plain data built by functions.
import type { EntitySpec, Player, TagOf, Vec2 } from "alpha";
import { vec } from "alpha";
import { BULLET, ENEMY, HERO, WALL } from "../tuning";
import type { Arena } from "../types";

type Spec<T extends TagOf<Arena>> = EntitySpec<Arena, T>;

// `data.player` says which player's input drives it; `aim` is the last direction moved.
export function hero(player: Player, at: Vec2): Spec<"hero"> {
  return {
    tags: ["hero"],
    pos: at,
    z: 2,
    shape: { sheet: "hero", animation: "idle", ...(player === 2 ? { tint: "#9be7ff" } : {}) },
    body: { type: "dynamic", fixedRotation: true, collider: { circle: HERO.radius } },
    // Each hero carries a lantern (config.lighting keeps the rest of the arena dim).
    light: { radius: 110, color: player === 2 ? "#9be7ff" : "#ffd9a0", intensity: 1.8 },
    data: { player, hp: HERO.hp, bombs: HERO.bombs, aim: [1, 0], cooldown: 0 },
  };
}

export function enemy(at: Vec2): Spec<"enemy"> {
  return {
    tags: ["enemy"],
    pos: at,
    z: 1,
    shape: { circle: ENEMY.radius, color: "#e74c3c", outline: "#7b241c" },
    body: { type: "dynamic", fixedRotation: true, collider: { circle: ENEMY.radius } },
  };
}

// A sensor: it passes through things and reports overlaps (ctx.onEnter).
export function bullet(from: Vec2, aim: Vec2): Spec<"bullet"> {
  return {
    tags: ["bullet"],
    pos: vec.add(from, vec.scale(aim, HERO.radius + 4)),
    shape: { circle: BULLET.radius, color: "#ffe14d" },
    light: { radius: 28, color: "#ffe14d" },
    body: { type: "dynamic", bullet: true, velocity: vec.scale(aim, BULLET.speed), collider: { circle: BULLET.radius, sensor: true } },
    data: { life: BULLET.life },
  };
}

// The four walls around the arena.
/** The arena's four walls along the edges of the visible world `[x, y, w, h]` (ctx.camera.view). */
export function walls([x, y, w, h]: readonly number[]): Spec<"wall">[] {
  const box = (pos: Vec2, size: Vec2): Spec<"wall"> => ({
    tags: ["wall"], pos, shape: { rect: size, color: "#34495e" }, body: { type: "static", collider: { rect: size } },
  });
  const cx = x + w / 2;
  const cy = y + h / 2;
  return [box([cx, y + WALL / 2], [w, WALL]), box([cx, y + h - WALL / 2], [w, WALL]),
          box([x + WALL / 2, cy], [WALL, h]), box([x + w - WALL / 2, cy], [WALL, h])];
}
