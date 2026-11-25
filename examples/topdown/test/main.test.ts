// Headless agent tests: same seed + same inputs => same result, every run.
import { simulate } from "alpha/test";
import game from "../src/main";

const frames = (from: number, to: number) => Array.from({ length: to - from }, (_, i) => from + i);
const score = (state: unknown) => (state as { score: number }).score;

export function startsWithOneHeroInAWalledArena() {
  const run = simulate(game, { seconds: 0.5 });
  run.expect.noErrors();
  run.expect.count("hero").equals(1);
  run.expect.count("wall").equals(4);
  run.expect.shown("hud");
}

export function enemiesKeepComing() {
  simulate(game, { seconds: 4 }).expect.count("enemy").atLeast(3);
}

export function walkingAnimatesAndMovesTheHero() {
  const run = simulate(game, { seconds: 0.5, inputs: { axes: { moveX: [[0, 1]] } } });
  run.expect.animation("hero").equals("walk");
  const [h] = run.entities("hero");
  if (h.pos[0] < 170) throw new Error(`hero should have walked right from x 140, at ${h.pos[0]}`);
}

export function holdingFireShootsAStreamThatExpires() {
  simulate(game, { seconds: 0.5, inputs: { fire: frames(0, 30) } }).expect.count("bullet").atLeast(2);
  simulate(game, { seconds: 2, inputs: { fire: frames(0, 30) } }).expect.count("bullet").equals(0);
}

export function shootingEnemiesScores() {
  // Shots auto-aim at the nearest enemy.
  const run = simulate(game, { seconds: 5, inputs: { fire: frames(0, 300) } });
  run.expect.noErrors();
  run.expect.scene("play");
  if (score(run.state) < 2) throw new Error(`expected some kills, score ${score(run.state)}`);
}

export function aBombClearsTheEnemiesAroundTheHero() {
  // At 4 s the first enemies are close; nothing else scores without firing.
  if (score(simulate(game, { seconds: 4.2 }).state) !== 0) throw new Error("no score without a bomb");
  const run = simulate(game, { seconds: 4.2, inputs: { bomb: [240] } });
  run.expect.noErrors();
  if (score(run.state) < 1) throw new Error(`bomb should kill nearby enemies, score ${score(run.state)}`);
  run.expect.text("hud", "bombs").equals("BOMBS 1");
  run.expect.count("blast").atLeast(1);
}

export function standingStillIsOverrunAndTheBestScoreIsSaved() {
  const run = simulate(game, { seconds: 40, saves: { main: { best: 5 } } });
  run.expect.scene("over");
  run.expect.shown("over");
  run.expect.text("over", "best").equals("BEST 5");
  if (run.saves.main.best !== 5) throw new Error(`best kept: ${JSON.stringify(run.saves)}`);
}

export function againStartsANewRound() {
  // Overrun before 40 s (previous test); the click lands on the shown "over" screen.
  const run = simulate(game, { seconds: 40.1, clicks: { "over#again": [2400] } });
  run.expect.noErrors();
  run.expect.scene("play");
}

export function lightsAreRenderOnly() {
  // Heroes and bullets carry lights; the simulation must not notice them.
  const run = simulate(game, { seconds: 1, inputs: { fire: frames(0, 60) } });
  run.expect.noErrors();
  run.expect.count("hero").equals(1);
  if (run.entities("bullet").length + score(run.state) === 0) throw new Error("firing should leave bullets or kills");
}
