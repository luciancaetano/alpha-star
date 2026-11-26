import type { Collider, EntitySpec, Random, Vec2 } from "alpha";
import type { Flappy } from "../types";
import { PIPE, WORLD } from "../tuning";

type Spec = EntitySpec<Flappy, "solid" | "goal" | "scrolls">;

/**
 * Top pipe, bottom pipe and a scoring sensor in the gap, just past the right edge of the visible
 * world. The gap range is the design's (same difficulty on every screen); the top pipe reaches up
 * to the top of the visible world, so taller screens show no open sky above it.
 */
export function pipePair(random: Random, [vx, vy, vw]: readonly number[]): Spec[] {
  const floor = WORLD.height - WORLD.groundHeight;
  const half = PIPE.gap / 2;
  const gapY = random.range(PIPE.margin + half, floor - PIPE.margin - half);
  const x = vx + vw + PIPE.width;
  return [pipe(x, Math.min(0, vy), gapY - half), pipe(x, gapY + half, floor), goal(x + PIPE.width / 2, gapY)];
}

function pipe(x: number, top: number, bottom: number): Spec {
  const size: Vec2 = [PIPE.width, bottom - top];
  return {
    tags: ["solid", "scrolls"],
    pos: [x, (top + bottom) / 2],
    shape: { image: "column", size },
    body: scrolling({ rect: size }),
  };
}

function goal(x: number, y: number): Spec {
  return { tags: ["goal", "scrolls"], pos: [x, y], body: scrolling({ rect: [4, PIPE.gap], sensor: true }) };
}

function scrolling(collider: Collider) {
  return { type: "kinematic", velocity: [-PIPE.speed, 0], collider } as const satisfies Spec["body"];
}
