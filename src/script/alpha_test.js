// Module "alpha/test": headless, deterministic runs of a game for agent tests.
// Types: alpha-sdk/types/alpha.d.ts. Each simulate() is a fresh game instance
// (own world, timers, seeded random, in-memory save), stepped at a fixed 1/60 s
// with no window and no audio output, so the same options give the same run.
import { createInstance, memorySave } from "alpha:runtime";

const N = globalThis.__native;
const DT = 1 / 60;

function fail(message) {
  throw new Error(message);
}

function framesOf(seconds) {
  if (typeof seconds !== "number" || !(seconds > 0)) fail(`simulate: seconds must be a positive number, got ${seconds}`);
  return Math.round(seconds / DT);
}

// { action: [frame, ...] } -> Map(frame -> [action, ...]), validated against the game.
function pressesByFrame(inputs, actions) {
  const byFrame = new Map();
  for (const [action, frames] of Object.entries(inputs ?? {})) {
    if (!actions.includes(action)) fail(`simulate: inputs.${action} is not an action; actions: ${actions.join(", ")}`);
    if (!Array.isArray(frames)) fail(`simulate: inputs.${action} must be an array of frame numbers`);
    for (const f of frames) {
      if (!Number.isInteger(f) || f < 0) fail(`simulate: inputs.${action} has ${f}; frame numbers are integers >= 0`);
      byFrame.set(f, [...(byFrame.get(f) ?? []), action]);
    }
  }
  return byFrame;
}

function describe(error) {
  return error ? `${error.message}\n${error.stack ?? ""}`.trimEnd() : "";
}

// Runs the game in a fresh instance and returns what tests may inspect.
function runGame(game, opts) {
  const frames = framesOf(opts.seconds);
  const id = N.instanceCreate();
  try {
    const sim = createInstance(id, memorySave());
    sim.boot(game, opts.scene);
    const presses = pressesByFrame(opts.inputs, sim.actions());
    let error = null;
    for (let f = 0; f < frames && !error; ++f) {
      N.stepBudget();
      try {
        sim.hold(presses.get(f) ?? []);
        sim.step(DT);
      } catch (e) {
        error = e;  // the game pauses here, as it would in the engine
      }
    }
    return { frames, scene: sim.scene(), counts: sim.tagCounts(), error };
  } finally {
    N.instanceDestroy(id);
  }
}

function expectations(r) {
  const after = `after ${r.frames} frames`;
  const paused = r.error ? ` (game paused by an error: ${describe(r.error)})` : "";
  const count = (tag) => r.counts[tag] ?? 0;
  return {
    scene(name) {
      if (r.scene !== name) fail(`expected scene "${name}" ${after}, got "${r.scene}"${paused}`);
    },
    noErrors() {
      if (r.error) fail(`expected no errors, got: ${describe(r.error)}`);
    },
    count: (tag) => ({
      equals(n) {
        if (count(tag) !== n) fail(`expected ${n} "${tag}" ${after}, got ${count(tag)}${paused}`);
      },
      atLeast(n) {
        if (count(tag) < n) fail(`expected at least ${n} "${tag}" ${after}, got ${count(tag)}${paused}`);
      },
    }),
  };
}

export function simulate(game, opts) {
  if (!opts || typeof opts !== "object") fail("simulate(game, { seconds, scene?, inputs? }): options are required");
  const r = runGame(game, opts);
  return { frames: r.frames, expect: expectations(r) };
}

// Test runner support: names of the exported test functions, sorted.
export function testNames(ns) {
  return Object.keys(ns).filter((k) => typeof ns[k] === "function").sort();
}
