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
  simulate(game, { seconds: 1.75, inputs: { jump: [60] } }).expect.animation("player").equals("fall");
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
