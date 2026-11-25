// ctx.tween: numeric properties eased over game time, advanced on the fixed
// step (deterministic). Embedded in the engine as "alpha:tween".

const pow = (p) => (t) => t ** p;
const sine = (t) => 1 - Math.cos((t * Math.PI) / 2);
const expo = (t) => (t === 0 ? 0 : 2 ** (10 * t - 10));
const circ = (t) => 1 - Math.sqrt(1 - t * t);
const back = (t) => 2.70158 * t * t * t - 1.70158 * t * t;
const elastic = (t) => (t === 0 || t === 1 ? t : -(2 ** (10 * t - 10)) * Math.sin((t * 10 - 10.75) * ((2 * Math.PI) / 3)));
function bounceOut(t) {
  const n = 7.5625;
  const d = 2.75;
  if (t < 1 / d) return n * t * t;
  if (t < 2 / d) return n * (t -= 1.5 / d) * t + 0.75;
  if (t < 2.5 / d) return n * (t -= 2.25 / d) * t + 0.9375;
  return n * (t -= 2.625 / d) * t + 0.984375;
}
const bounce = (t) => 1 - bounceOut(1 - t);

// Standard Penner easings: name + In / Out / InOut, plus "linear".
export const easings = { linear: (t) => t };
const ins = { quad: pow(2), cubic: pow(3), quart: pow(4), quint: pow(5), sine, expo, circ, back, elastic, bounce };
for (const [name, fin] of Object.entries(ins)) {
  const out = (t) => 1 - fin(1 - t);
  easings[`${name}In`] = fin;
  easings[`${name}Out`] = out;
  easings[`${name}InOut`] = (t) => (t < 0.5 ? fin(2 * t) / 2 : 0.5 + out(2 * t - 1) / 2);
}

function fail(message) {
  throw new Error(message);
}

function checkOptions(opts) {
  if (!opts || typeof opts !== "object" || !opts.to || typeof opts.to !== "object") {
    fail("tween(target, { to: { prop: value }, seconds, ease?, delay?, yoyo?, repeat?, onDone? }): `to` is required");
  }
  if (!(opts.seconds > 0)) fail(`tween: seconds must be a number > 0, got ${JSON.stringify(opts.seconds)}`);
  const ease = opts.ease ?? "linear";
  if (!Object.hasOwn(easings, ease)) fail(`tween: unknown ease "${ease}"; eases: ${Object.keys(easings).join(", ")}`);
  const repeat = opts.repeat ?? 0;
  if (!(repeat === Infinity || (Number.isInteger(repeat) && repeat >= 0))) fail(`tween: repeat must be an integer >= 0 or Infinity, got ${repeat}`);
  if (opts.onDone !== undefined && typeof opts.onDone !== "function") fail("tween: onDone must be a function");
  for (const [k, v] of Object.entries(opts.to)) {
    if (typeof v !== "number" || !Number.isFinite(v)) fail(`tween: to.${k} must be a finite number, got ${JSON.stringify(v)}`);
  }
  return { ease: easings[ease], repeat, delay: opts.delay ?? 0 };
}

// Progress 0..1 of the current cycle at `time` (seconds since start), or null when finished.
function progress(t, time) {
  const cycle = t.seconds * (t.yoyo ? 2 : 1);
  const n = Math.floor(time / cycle);
  if (n > t.repeat) return null;
  const local = time - n * cycle;
  const forward = Math.min(local, t.seconds) / t.seconds;
  return t.yoyo && local > t.seconds ? 1 - (local - t.seconds) / t.seconds : forward;
}

// `access(target)` -> { check(keys), get(key), set(key, value, dt), settle(key) } for that
// target; settle runs the step after a tween ends (kinematic bodies stop there).
export function makeTweens(access) {
  let list = [];
  let settling = [];
  function start(t) {
    t.props = access(t.target);
    for (const key of Object.keys(t.to)) t.from[key] = t.props.get(key);
    t.started = true;
  }
  function apply(t, p, dt) {
    for (const [key, end] of Object.entries(t.to)) t.props.set(key, t.from[key] + (end - t.from[key]) * t.ease(p), dt);
  }
  function advance(t, dt) {
    if (t.cancelled || !t.alive()) return false;
    t.time += dt;
    if (t.time < t.delay) return true;
    if (!t.started) start(t);
    const time = t.time - t.delay;
    const p = progress(t, time);
    const finalP = t.yoyo ? 0 : 1;
    apply(t, p ?? finalP, dt);
    if (p !== null) return true;
    t.done = true;
    settling.push(t);  // settle after the world moved it this step
    t.onDone?.();
    return false;
  }
  return {
    add(target, opts, alive) {
      const o = checkOptions(opts);
      access(target).check(Object.keys(opts.to));
      const t = { target, to: { ...opts.to }, from: {}, seconds: opts.seconds, yoyo: Boolean(opts.yoyo), ...o,
        onDone: opts.onDone, time: 0, started: false, done: false, cancelled: false, alive };
      list.push(t);
      return {
        cancel() { t.cancelled = true; },
        get done() { return t.done; },
        get active() { return !t.done && !t.cancelled; },
      };
    },
    // One fixed step; tweens added during it start next step.
    step(dt) {
      for (const t of settling) if (t.alive()) for (const key of Object.keys(t.to)) t.props.settle(key);
      settling = [];
      const now = list;
      list = [];
      const kept = now.filter((t) => advance(t, dt));
      list = kept.concat(list);
    },
    clear() {
      list = [];
      settling = [];
    },
    get count() { return list.length; },
  };
}
