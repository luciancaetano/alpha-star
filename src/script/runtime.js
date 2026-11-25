// alpha-star script runtime: scenes, ctx, entities. Embedded in the engine;
// game scripts never import it. Everything engine-side goes through __native,
// which validates its arguments and throws readable errors.
const N = globalThis.__native;

Math.random = () => {
  throw new Error("Math.random is disabled so runs are reproducible; use ctx.random");
};

let game = null;
let current = null;
let pending = null;
let texts = [];
let handlers = [];
let saveData = {};
let sceneTime = 0;
let stepDt = 1 / 60;
const timers = new Map();
const entities = new Map();

function fail(message) {
  throw new Error(message);
}

function sceneNames() {
  return Object.keys(game.scenes).join(", ");
}

// ---- entities ---------------------------------------------------------------

function entity(id) {
  let e = entities.get(id);
  if (e) return e;
  e = {
    id,
    get alive() { return N.alive(id); },
    get tags() { return N.tags(id); },
    pos: {
      get x() { return N.position(id)[0]; },
      set x(v) { N.setPosition(id, v, N.position(id)[1]); },
      get y() { return N.position(id)[1]; },
      set y(v) { N.setPosition(id, N.position(id)[0], v); },
    },
    get rotation() { return N.rotation(id); },
    set rotation(v) { N.setRotation(id, v); },
    get z() { return N.z(id); },
    set z(v) { N.setZ(id, v); },
    body: {
      get velocity() {
        const [x, y] = N.velocity(id);
        return { x, y };
      },
      set velocity(v) {
        if (!Array.isArray(v)) fail(`entity ${id}: body.velocity must be set to [x, y]`);
        N.setVelocity(id, v[0], v[1]);
      },
    },
  };
  entities.set(id, e);
  return e;
}

// ---- ctx ----------------------------------------------------------------------

let masterVolume = 1;

function playArgs(opts = {}) {
  return [opts.volume ?? 1, opts.pitch ?? 1, Boolean(opts.loop)];
}

function voice(id) {
  return {
    get playing() { return N.voicePlaying(id); },
    stop: () => N.stopVoice(id),
  };
}

function addTimer(id, fn) {
  if (typeof fn !== "function") fail("timer callback must be a function");
  timers.set(id, fn);
}

const ctx = {
  get config() { return game.config; },
  get dt() { return stepDt; },
  get time() { return sceneTime; },
  state: undefined,
  input: {
    pressed: (action) => N.action(action, 0),
    down: (action) => N.action(action, 1),
    released: (action) => N.action(action, 2),
  },
  random: {
    range: (min, max) => N.random(min, max, false),
    int: (min, max) => N.random(min, max, true),
    pick: (items) => items[N.random(0, items.length - 1, true)],
  },
  save: {
    get(key, fallback) {
      return Object.hasOwn(saveData, key) ? saveData[key] : fallback;
    },
    set(key, value) {
      saveData[key] = value;
      N.saveWrite(JSON.stringify(saveData));
    },
  },
  ui: {
    text(content, opts) {
      if (!opts || !Array.isArray(opts.pos)) fail("ui.text(content, opts): opts.pos must be [x, y]");
      texts.push({ content, opts });
    },
  },
  audio: {
    play: (sound, opts) => voice(N.playSound(sound, ...playArgs(opts))),
    tone: (synth, opts) => voice(N.playTone(synth, ...playArgs(opts))),
    stopAll: () => N.stopAll(),
    get volume() { return masterVolume; },
    set volume(v) {
      N.setVolume(v);
      masterVolume = v;
    },
  },
  spawn: (spec) => entity(N.spawn(spec)),
  destroy: (e) => N.destroy(e.id),
  find(tag) {
    const ids = N.findAll(tag);
    if (!ids.length) fail(`find("${tag}"): no living entity has this tag`);
    return entity(ids[0]);
  },
  findAll: (tag) => N.findAll(tag).map(entity),
  onHit: (tagA, tagB, fn) => handlers.push({ kind: "hits", tagA, tagB, fn }),
  onEnter: (tagA, tagB, fn) => handlers.push({ kind: "enters", tagA, tagB, fn }),
  every: (seconds, fn) => addTimer(N.timerEvery(seconds), fn),
  after(seconds, fn) {
    const id = N.timerAfter(seconds);
    addTimer(id, () => {
      timers.delete(id);
      fn();
    });
  },
  go(name, params) {
    if (!Object.hasOwn(game.scenes, name)) fail(`go("${name}"): unknown scene; scenes: ${sceneNames()}`);
    pending = { name, params };
  },
};

// ---- scenes -------------------------------------------------------------------

function enter(name, params) {
  current?.def.exit?.(ctx);
  N.resetWorld();
  N.clearTimers();
  timers.clear();
  entities.clear();
  handlers = [];
  texts = [];
  sceneTime = 0;
  current = game.scenes[name];
  ctx.state = current.def.state ? current.def.state(params) : undefined;
  current.def.enter?.(ctx, params);
}

function dispatch(pairs, kind) {
  for (const [a, b] of pairs) {
    for (const h of handlers) {
      if (h.kind !== kind || !N.alive(a) || !N.alive(b)) continue;
      if (N.hasTag(a, h.tagA) && N.hasTag(b, h.tagB)) h.fn(entity(a), entity(b));
      else if (N.hasTag(b, h.tagA) && N.hasTag(a, h.tagB)) h.fn(entity(b), entity(a));
    }
  }
}

function validate(g) {
  if (!g || typeof g !== "object" || !g.def) {
    fail("main module must `export default game({...})`, using game from define<...>() in \"alpha\"");
  }
  const def = g.def;
  if (!def.config || !Array.isArray(def.config.size)) fail("game config.size must be [width, height]");
  if (!def.scenes || typeof def.scenes !== "object") fail("game scenes must be an object { name: scene }");
  for (const [key, s] of Object.entries(def.scenes)) {
    if (!s || s.name !== key || !s.def) fail(`scenes.${key} must be scene("${key}", {...})`);
  }
  if (!Object.hasOwn(def.scenes, def.start)) fail(`start "${def.start}" is not a scene; scenes: ${Object.keys(def.scenes).join(", ")}`);
  return def;
}

// Every asset is loaded (or synthesized) before the first scene starts.
function preload(assets) {
  for (const [name, sound] of Object.entries(assets.sounds ?? {})) {
    if (typeof sound === "string") N.loadSound(name, sound);
    else N.synthSound(name, sound);
  }
  for (const [name, file] of Object.entries(assets.images ?? {})) N.loadImage(name, file);
}

// ---- host entry points (called by the engine) --------------------------------

export function boot(g) {
  game = validate(g);
  const c = game.config;
  N.configure(c.seed ?? 0, ...(c.gravity ?? [0, 0]));
  for (const [action, inputs] of Object.entries(c.actions ?? {})) N.bindAction(action, inputs);
  preload(game.assets ?? {});
  saveData = JSON.parse(N.saveRead(c.title ?? "alpha-star") || "{}");
  enter(game.start, undefined);
  return { title: c.title ?? "alpha-star", size: c.size, background: c.background ?? "#000000" };
}

export function step(dt) {
  stepDt = dt;
  current.def.update?.(ctx);
  for (const id of N.advanceTimers(dt)) timers.get(id)?.();
  const events = N.stepWorld(dt);
  sceneTime += dt;
  dispatch(events.hits, "hits");
  dispatch(events.enters, "enters");
  if (pending) {
    const { name, params } = pending;
    pending = null;
    enter(name, params);
  }
}

export function frameTexts() {
  return texts.map(({ content, opts }) => ({
    text: String(typeof content === "function" ? content() : content),
    x: opts.pos[0],
    y: opts.pos[1],
    size: opts.size ?? 16,
    color: opts.color ?? "#ffffff",
    align: opts.align ?? "left",
  }));
}
