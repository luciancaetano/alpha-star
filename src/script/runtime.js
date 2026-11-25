// alpha-star script runtime: scenes, ctx, entities. Embedded in the engine;
// game scripts never import it. Everything engine-side goes through __native,
// which validates its arguments and throws readable errors.
//
// A game runs as an *instance*: all of its state lives in the closure made by
// createInstance, and its native state (world, timers, random, assets) lives
// in the engine under the same instance id. The engine's own game is instance
// 0; every simulate() run (alpha/test) is a fresh instance, so runs never see
// each other. Every call into an instance selects it natively first.
const N = globalThis.__native;

Math.random = () => {
  throw new Error("Math.random is disabled so runs are reproducible; use ctx.random");
};

function fail(message) {
  throw new Error(message);
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

// ctx.save backends: the engine's save file, or memory only (simulate).
export const fileSave = {
  read: (title) => JSON.parse(N.saveRead(title) || "{}"),
  write: (data) => N.saveWrite(JSON.stringify(data)),
};

export function memorySave() {
  return { read: () => ({}), write: () => {} };
}

function makeEntities() {
  const entities = new Map();
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
  return { entity, clear: () => entities.clear() };
}

function voice(id) {
  return {
    get playing() { return N.voicePlaying(id); },
    stop: () => N.stopVoice(id),
  };
}

function playArgs(opts = {}) {
  return [opts.volume ?? 1, opts.pitch ?? 1, Boolean(opts.loop)];
}

// The ctx object a game's scenes see. `s` is the instance's mutable state.
function makeCtx(s) {
  const addTimer = (id, fn) => {
    if (typeof fn !== "function") fail("timer callback must be a function");
    s.timers.set(id, fn);
  };
  return {
    get config() { return s.game.config; },
    get dt() { return s.stepDt; },
    get time() { return s.sceneTime; },
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
      get: (key, fallback) => (Object.hasOwn(s.saveData, key) ? s.saveData[key] : fallback),
      set(key, value) {
        s.saveData[key] = value;
        s.save.write(s.saveData);
      },
    },
    ui: {
      text(content, opts) {
        if (!opts || !Array.isArray(opts.pos)) fail("ui.text(content, opts): opts.pos must be [x, y]");
        s.texts.push({ content, opts });
      },
    },
    audio: {
      play: (sound, opts) => voice(N.playSound(sound, ...playArgs(opts))),
      tone: (synth, opts) => voice(N.playTone(synth, ...playArgs(opts))),
      stopAll: () => N.stopAll(),
      get volume() { return s.masterVolume; },
      set volume(v) {
        N.setVolume(v);
        s.masterVolume = v;
      },
    },
    spawn: (spec) => s.entities.entity(N.spawn(spec)),
    destroy: (e) => N.destroy(e.id),
    find(tag) {
      const ids = N.findAll(tag);
      if (!ids.length) fail(`find("${tag}"): no living entity has this tag`);
      return s.entities.entity(ids[0]);
    },
    findAll: (tag) => N.findAll(tag).map(s.entities.entity),
    onHit: (tagA, tagB, fn) => s.handlers.push({ kind: "hits", tagA, tagB, fn }),
    onEnter: (tagA, tagB, fn) => s.handlers.push({ kind: "enters", tagA, tagB, fn }),
    every: (seconds, fn) => addTimer(N.timerEvery(seconds), fn),
    after(seconds, fn) {
      const id = N.timerAfter(seconds);
      addTimer(id, () => {
        s.timers.delete(id);
        fn();
      });
    },
    go(name, params) {
      s.pending = { name: checkScene(s.game, name, "go"), params };
    },
  };
}

function checkScene(game, name, what) {
  if (!Object.hasOwn(game.scenes, name)) {
    fail(`${what}("${name}"): scene "${name}" is not a scene; scenes: ${Object.keys(game.scenes).join(", ")}`);
  }
  return name;
}

function enter(s, ctx, name, params) {
  s.current?.def.exit?.(ctx);
  N.resetWorld();
  N.clearTimers();
  s.timers.clear();
  s.entities.clear();
  s.handlers = [];
  s.texts = [];
  s.sceneTime = 0;
  s.current = s.game.scenes[name];
  s.currentName = name;
  ctx.state = s.current.def.state ? s.current.def.state(params) : undefined;
  s.current.def.enter?.(ctx, params);
}

function dispatch(s, pairs, kind) {
  for (const [a, b] of pairs) {
    for (const h of s.handlers) {
      if (h.kind !== kind || !N.alive(a) || !N.alive(b)) continue;
      if (N.hasTag(a, h.tagA) && N.hasTag(b, h.tagB)) h.fn(s.entities.entity(a), s.entities.entity(b));
      else if (N.hasTag(b, h.tagA) && N.hasTag(a, h.tagB)) h.fn(s.entities.entity(b), s.entities.entity(a));
    }
  }
}

function bootGame(s, ctx, g, start) {
  s.game = validate(g);
  const c = s.game.config;
  N.configure(c.seed ?? 0, ...(c.gravity ?? [0, 0]));
  for (const [action, inputs] of Object.entries(c.actions ?? {})) N.bindAction(action, inputs);
  preload(s.game.assets ?? {});
  s.saveData = s.save.read(c.title ?? "alpha-star");
  enter(s, ctx, start === undefined ? s.game.start : checkScene(s.game, start, "simulate"), undefined);
  return { title: c.title ?? "alpha-star", size: c.size, background: c.background ?? "#000000" };
}

function stepGame(s, ctx, dt) {
  s.stepDt = dt;
  s.current.def.update?.(ctx);
  for (const id of N.advanceTimers(dt)) s.timers.get(id)?.();
  const events = N.stepWorld(dt);
  s.sceneTime += dt;
  dispatch(s, events.hits, "hits");
  dispatch(s, events.enters, "enters");
  if (s.pending) {
    const { name, params } = s.pending;
    s.pending = null;
    enter(s, ctx, name, params);
  }
}

function textsOf(s) {
  return s.texts.map(({ content, opts }) => ({
    text: String(typeof content === "function" ? content() : content),
    x: opts.pos[0],
    y: opts.pos[1],
    size: opts.size ?? 16,
    color: opts.color ?? "#ffffff",
    align: opts.align ?? "left",
  }));
}

// Runs fn with native instance `id` active, restoring the previous one after.
function within(id, fn) {
  const previous = N.instanceUse(id);
  try {
    return fn();
  } finally {
    N.instanceUse(previous);
  }
}

// One game instance. `id` is its native instance; `save` its ctx.save backend.
export function createInstance(id, save) {
  const s = {
    game: null, current: null, currentName: null, pending: null, texts: [], handlers: [],
    saveData: {}, save, sceneTime: 0, stepDt: 1 / 60, masterVolume: 1,
    timers: new Map(), entities: makeEntities(),
  };
  const ctx = makeCtx(s);
  return {
    boot: (g, start) => within(id, () => bootGame(s, ctx, g, start)),
    step: (dt) => within(id, () => stepGame(s, ctx, dt)),
    frameTexts: () => within(id, () => textsOf(s)),
    hold: (actions) => within(id, () => N.holdActions(actions)),
    actions: () => Object.keys(s.game?.config.actions ?? {}),
    scene: () => s.currentName,
    tagCounts: () => within(id, () => N.tagCounts()),
  };
}

// ---- host entry points (the engine's own game, instance 0) -------------------

const main = createInstance(0, fileSave);

export function boot(g) {
  return main.boot(g);
}

export function step(dt) {
  main.step(dt);
}

export function frameTexts() {
  return main.frameTexts();
}
