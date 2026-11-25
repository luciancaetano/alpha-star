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
function pressesByFrame(inputs, actions, label) {
  const byFrame = new Map();
  for (const [action, frames] of Object.entries(inputs)) {
    if (!actions.includes(action)) fail(`simulate: ${label}.${action} is not an action; actions: ${actions.join(", ")}`);
    if (!Array.isArray(frames)) fail(`simulate: ${label}.${action} must be an array of frame numbers`);
    for (const f of frames) {
      if (!Number.isInteger(f) || f < 0) fail(`simulate: ${label}.${action} has ${f}; frame numbers are integers >= 0`);
      byFrame.set(f, [...(byFrame.get(f) ?? []), action]);
    }
  }
  return byFrame;
}

// { axis: [[frame, value], ...] } -> [[axis, points]], validated against the game.
function axisTimelines(axes, declared, label) {
  const out = [];
  for (const [axis, points] of Object.entries(axes ?? {})) {
    if (!declared.includes(axis)) fail(`simulate: ${label}.axes.${axis} is not an axis; axes: ${declared.join(", ") || "(none)"}`);
    if (!Array.isArray(points)) fail(`simulate: ${label}.axes.${axis} must be an array of [frame, value]`);
    let last = -1;
    for (const pt of points) {
      const ok = Array.isArray(pt) && pt.length === 2 && Number.isInteger(pt[0]) && pt[0] > last &&
        typeof pt[1] === "number" && pt[1] >= -1 && pt[1] <= 1;
      if (!ok) fail(`simulate: ${label}.axes.${axis} has ${JSON.stringify(pt)}; entries are [frame, value] with increasing integer frames >= 0 and -1 <= value <= 1`);
      last = pt[0];
    }
    out.push([axis, points]);
  }
  return out;
}

// inputs: one object (player 1) or an array with one object per player.
function playerInputs(inputs, sim) {
  const list = inputs === undefined ? [{}] : Array.isArray(inputs) ? inputs : [inputs];
  if (list.length < 1 || list.length > 4) fail(`simulate: inputs as an array has one object per player (1..4 objects), got ${list.length}`);
  return list.map((p, i) => {
    const label = Array.isArray(inputs) ? `inputs[${i}]` : "inputs";
    const { axes, ...actions } = p ?? {};
    return { presses: pressesByFrame(actions, sim.actions(), label), axes: axisTimelines(axes, sim.axes(), label) };
  });
}

// The value an axis timeline holds at frame f (undefined before its first entry).
function axisAt(points, f) {
  let v;
  for (const [frame, value] of points) {
    if (frame > f) break;
    v = value;
  }
  return v;
}

function holdFrame(players, f) {
  const held = [0, 1, 2, 3].map((p) => players[p]?.presses.get(f) ?? []);
  N.holdActions(...held, players.length);
  players.forEach((player, p) => {
    for (const [axis, points] of player.axes) {
      const v = axisAt(points, f);
      if (v !== undefined) N.holdAxis(axis, p + 1, v);
    }
  });
}

// { "doc#id": [frame, ...] } -> Map(frame -> [[doc, id], ...]).
function clicksByFrame(clicks) {
  const byFrame = new Map();
  for (const [target, frames] of Object.entries(clicks ?? {})) {
    const [doc, id, extra] = target.split("#");
    if (!doc || !id || extra !== undefined) fail(`simulate: clicks key "${target}" must be "document#elementId"`);
    if (!Array.isArray(frames)) fail(`simulate: clicks["${target}"] must be an array of frame numbers`);
    for (const f of frames) {
      if (!Number.isInteger(f) || f < 0) fail(`simulate: clicks["${target}"] has ${f}; frame numbers are integers >= 0`);
      byFrame.set(f, [...(byFrame.get(f) ?? []), [doc, id]]);
    }
  }
  return byFrame;
}

// A click the test asked for; a hidden document or unknown id fails the test.
function click(sim, f, [doc, id]) {
  try {
    sim.within(() => N.uiClick(doc, id));
  } catch (e) {
    fail(`simulate: clicks["${doc}#${id}"] at frame ${f}: ${e.message}`);
  }
}

function describe(error) {
  return error ? `${error.message}\n${error.stack ?? ""}`.trimEnd() : "";
}

// Tag -> animation of its oldest living entity ("" when it has no sheet).
function animationsOf(counts) {
  const out = {};
  for (const tag of Object.keys(counts)) {
    const ids = N.findAll(tag);
    if (ids.length) out[tag] = N.animation(ids[0]);
  }
  return out;
}

// Runs the game in a fresh instance and returns what tests may inspect.
function runGame(game, opts) {
  const frames = framesOf(opts.seconds);
  const id = N.instanceCreate();
  try {
    const sim = createInstance(id, memorySave());
    sim.boot(game, opts.scene, "simulate");
    const players = playerInputs(opts.inputs, sim);
    const clicks = clicksByFrame(opts.clicks);
    let error = null;
    for (let f = 0; f < frames && !error; ++f) {
      N.stepBudget();
      for (const target of clicks.get(f) ?? []) click(sim, f, target);
      try {
        sim.within(() => holdFrame(players, f));
        sim.step(DT);
      } catch (e) {
        error = e;  // the game pauses here, as it would in the engine
      }
    }
    const counts = sim.within(() => N.tagCounts());
    return { frames, scene: sim.scene(), counts, animations: sim.within(() => animationsOf(counts)), ui: sim.within(() => N.uiView()),
      particles: sim.within(() => N.particles()), error };
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
    text: (doc, id) => ({
      equals(text) {
        const d = r.ui[doc];
        if (!d) fail(`expect.text("${doc}", "${id}"): unknown document; assets.ui has: ${Object.keys(r.ui).join(", ") || "(none)"}`);
        if (!Object.hasOwn(d.texts, id)) fail(`expect.text("${doc}", "${id}"): no element with that id; ids: ${Object.keys(d.texts).join(", ") || "(none)"}`);
        const got = d.texts[id];
        const hidden = d.shown ? "" : ` (document "${doc}" is hidden)`;
        if (got !== text) fail(`expected ${doc}#${id} text "${text}" ${after}, got "${got}"${hidden}${paused}`);
      },
    }),
    animation: (tag) => ({
      equals(name) {
        if (!Object.hasOwn(r.animations, tag)) fail(`expect.animation("${tag}"): no living entity has this tag ${after}${paused}`);
        const got = r.animations[tag];
        if (got !== name) fail(`expected "${tag}" playing animation "${name}" ${after}, got ${got ? `"${got}"` : "no animation (not a sheet sprite)"}${paused}`);
      },
    }),
    shown(doc) {
      if (!r.ui[doc]?.shown) fail(`expected document "${doc}" shown ${after}; shown: ${Object.keys(r.ui).filter((k) => r.ui[k].shown).join(", ") || "(none)"}${paused}`);
    },
    particles: () => ({
      equals(n) {
        if (r.particles !== n) fail(`expected ${n} live particles ${after}, got ${r.particles}${paused}`);
      },
      atLeast(n) {
        if (r.particles < n) fail(`expected at least ${n} live particles ${after}, got ${r.particles}${paused}`);
      },
    }),
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
  if (!opts || typeof opts !== "object") fail("simulate(game, { seconds, scene?, inputs?, clicks? }): options are required");
  const r = runGame(game, opts);
  return { frames: r.frames, expect: expectations(r) };
}

// Test runner support: names of the exported test functions, sorted.
export function testNames(ns) {
  return Object.keys(ns).filter((k) => typeof ns[k] === "function").sort();
}
