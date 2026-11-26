// Headless agent tests. The cycling rules are pure functions, tested directly; the game through
// simulate(). Shaders are render-only: headless runs use the software renderer, which skips them,
// but every shader is still loaded at boot and every ctx.postfx call is validated against its
// declared uniforms, so a bad asset or uniform name fails here.
import { simulate } from "alpha/test";
import { adjust, cycle, label, level } from "../src/cycle";
import game from "../src/main";
import { SHADERS } from "../src/tuning";

function check(ok: boolean, what: string) {
  if (!ok) throw new Error(what);
}

const state = (s: unknown) => s as { index: number; intensity: number };
const every = (n: number, gap: number, from = 5) => Array.from({ length: n }, (_, i) => from + i * gap);

export function cycleWrapsBothWays() {
  check(cycle(0, 1) === 1, "next from the first is the second");
  check(cycle(SHADERS.length - 1, 1) === 0, "next from the last wraps to the first");
  check(cycle(0, -1) === SHADERS.length - 1, "prev from the first wraps to the last");
  check(cycle(2, SHADERS.length) === 2, "a full lap comes back");
}

export function intensityMovesInQuartersAndStaysInRange() {
  check(adjust(0.75, 1) === 1 && adjust(1, 1) === 1, "capped at 1");
  check(adjust(0.25, -1) === 0 && adjust(0, -1) === 0, "floored at 0");
  check(adjust(0.5, -1) === 0.25, "one step down");
  check(level(0.25) === "INTENSITY 25%" && label(0) === "WAVE 1/6", `${level(0.25)} / ${label(0)}`);
}

export function everyShaderIsListedOnce() {
  check(new Set(SHADERS).size === SHADERS.length && SHADERS.length === 6, `SHADERS: ${SHADERS.join(" ")}`);
}

export function startsOnTheFirstShaderWithOrbsAndSparks() {
  const run = simulate(game, { seconds: 0.5 });
  run.expect.noErrors();
  run.expect.count("orb").equals(5);
  run.expect.particles().atLeast(10);
  run.expect.text("hud", "name").equals("WAVE 1/6");
  run.expect.text("hud", "level").equals("INTENSITY 75%");
}

// A full lap through every shader at every intensity: each add/set is checked against the
// shader's declared uniforms, so this fails on any asset or uniform mismatch.
export function aFullLapAtEveryIntensityHasNoErrors() {
  const run = simulate(game, { seconds: 4, inputs: { next: every(SHADERS.length * 2, 15), less: every(4, 40, 8) } });
  run.expect.noErrors();
  check(state(run.state).index === 0 && state(run.state).intensity === 0, `state: ${JSON.stringify(run.state)}`);
}

export function prevWrapsToTheLastShader() {
  const run = simulate(game, { seconds: 0.5, inputs: { prev: [5] } });
  run.expect.text("hud", "name").equals("PULSE 6/6");
}

export function upRaisesIntensityToTheCap() {
  const run = simulate(game, { seconds: 0.5, inputs: { more: [5, 10, 15] } });
  run.expect.text("hud", "level").equals("INTENSITY 100%");
}

export function orbsCircleOverTime() {
  const a = simulate(game, { seconds: 0.1 }).entities("orb")[0].pos;
  const b = simulate(game, { seconds: 1 }).entities("orb")[0].pos;
  check(a[0] !== b[0] || a[1] !== b[1], `orb did not move: ${a} -> ${b}`);
}

export function phonesGetOnScreenButtons() {
  simulate(game, { seconds: 0.2, platform: "windows" }).expect.text("hud", "hint").equals("LEFT/RIGHT: SHADER  UP/DOWN: INTENSITY");
  const run = simulate(game, { seconds: 0.5, platform: "android", clicks: { "touch#next": [5], "touch#less": [10] } });
  run.expect.shown("touch");
  run.expect.text("hud", "name").equals("CHROMA 2/6");
  run.expect.text("hud", "level").equals("INTENSITY 50%");
}
