// Headless agent tests: same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import game from "../src/main";

// moveX held at +1 from frame 0 (the D / Right key, or the stick).
const RIGHT = { axes: { moveX: [[0, 1]] as Array<[number, number]> } };

export function playerLandsAndIdles() {
  const run = simulate(game, { seconds: 1.5 });
  run.expect.noErrors();
  run.expect.animation("player").equals("idle");
  run.expect.count("coin").equals(8);
}

export function holdingRightRuns() {
  simulate(game, { seconds: 1.5, inputs: RIGHT }).expect.animation("player").equals("run");
}

export function jumpingShowsJumpThenFall() {
  simulate(game, { seconds: 1.2, inputs: { jump: [60] } }).expect.animation("player").equals("jump");
  simulate(game, { seconds: 1.55, inputs: { jump: [60] } }).expect.animation("player").equals("fall");
}

export function runningRightCollectsTheFirstCoins() {
  // Coins at x 100 and 130 lie on the ground path from the start (x 40).
  const run = simulate(game, { seconds: 1.5, inputs: RIGHT });
  run.expect.count("coin").equals(6);
  run.expect.text("hud", "coins").equals("coins 2/8");
}

export function mapCollidersAreMergedBoxes() {
  // Ground rows, three platforms and two side walls become a handful of static boxes.
  const run = simulate(game, { seconds: 0.1 });
  run.expect.count("walls").atLeast(5);
  run.expect.scene("play");
}

export function landingRaisesDustThatClearsItself() {
  // The hero lands ~0.15 s in: a one-shot puff whose entity goes away with its last particle.
  const run = simulate(game, { seconds: 0.3 });
  run.expect.count("dust").equals(1);
  run.expect.particles().atLeast(5);
  const later = simulate(game, { seconds: 1 });
  later.expect.count("dust").equals(0);
  later.expect.particles().equals(0);
}

export function pickedCoinsFloatAPlusOneThatFades() {
  simulate(game, { seconds: 0.6, inputs: RIGHT }).expect.count("popup").atLeast(1);
  simulate(game, { seconds: 2, inputs: RIGHT }).expect.count("popup").equals(0);
}

export function heroNeverTipsOver() {
  // fixedRotation on the body: no rotation reset in update, the box stays upright.
  const run = simulate(game, { seconds: 3, inputs: { jump: [60, 120], axes: { moveX: [[0, 1]] } } });
  for (const p of run.entities("player")) if (p.rotation !== 0) throw new Error(`player rotated to ${p.rotation}`);
}

export function jumpingUpThroughTheOneWayLedgeLandsOnIt() {
  // Run right under the plank (x 96..144, top y 121), jump and stop: up through it, then onto it.
  const run = simulate(game, { seconds: 1.5, inputs: { jump: [50], axes: { moveX: [[0, 1], [52, 0]] } } });
  const [hero] = run.entities("player");
  if (Math.abs(hero.pos[1] - 113) > 1) throw new Error(`expected the hero standing on the ledge (y 113), got y ${hero.pos[1]}`);
  run.expect.animation("player").equals("idle");  // the raycast ground check sees the ledge
}

export function theLiftGlidesOverThePitAndBack() {
  const at = (seconds: number) => simulate(game, { seconds }).entities("lift")[0].pos[0];
  if (Math.abs(at(2.5) - 470) > 1) throw new Error(`lift should reach x 470 at 2.5 s, got ${at(2.5)}`);
  if (Math.abs(at(5) - 340) > 1) throw new Error(`lift should be back at x 340 at 5 s, got ${at(5)}`);
  const mid = at(1.25);
  if (mid < 380 || mid > 430) throw new Error(`lift should be mid-way at 1.25 s, got ${mid}`);
}

export function aPickedCoinGrowsAndFadesBeforeGoingAway() {
  // The first coin is touched ~0.5 s in: it counts at once, then tweens out over 0.25 s.
  const popping = simulate(game, { seconds: 0.6, inputs: RIGHT });
  const [first] = popping.entities("coin");
  if (!first.data.taken || first.scale <= 1 || first.alpha >= 1) throw new Error(`coin not popping: ${JSON.stringify(first)}`);
  popping.expect.text("hud", "coins").equals("coins 1/8");
  simulate(game, { seconds: 0.9, inputs: RIGHT }).expect.count("coin").equals(7);
}
