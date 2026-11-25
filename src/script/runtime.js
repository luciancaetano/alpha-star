// alpha-star script runtime: scenes, ctx, entities. Embedded in the engine;
// game scripts never import it. Everything engine-side goes through __native,
// which validates its arguments and throws readable errors.
//
// A game runs as an *instance*: all of its state lives in the closure made by
// createInstance, and its native state (world, timers, random, assets) lives
// in the engine under the same instance id. The engine's own game is instance
// 0; every headless test run is a fresh instance, so runs never see
// each other. Every call into an instance selects it natively first.
import { joinPath, loadMap, mapView, spawnLayers } from "alpha:maps";
import { aliveOf, bodyOf, contactOf, physicsCtx, tweenAccess } from "alpha:physics";
import { makeTweens } from "alpha:tween";

const N = globalThis.__native;

// console.* goes to stderr and the dev tools console (levels 0 log, 1 warn, 2 error).
function show(value) {
  if (typeof value === "string") return value;
  try {
    return JSON.stringify(value) ?? String(value);
  } catch {
    return String(value);
  }
}
const writer = (level) => (...args) => N.log(level, args.map(show).join(" "));
globalThis.console = { log: writer(0), warn: writer(1), error: writer(2) };

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
  if (Object.hasOwn(def.config.actions ?? {}, "axes")) fail("config.actions.axes: the name \"axes\" is reserved (headless test inputs use it for analog axes); rename the action");
  if (!Object.hasOwn(def.scenes, def.start)) fail(`start "${def.start}" is not a scene; scenes: ${Object.keys(def.scenes).join(", ")}`);
  return def;
}

// `alpha export` may pack images into atlas pages: alpha-atlas.json maps each image
// file to [page file, [x, y, w, h]], crisp images in `images` and smooth ones
// (pixelMode false) in `smooth`, on separate pages. Without it (tests, raw files)
// every image is its own file. Returns (file, pixelMode) -> [file, region?].
function atlasOf() {
  const text = N.readText("alpha-atlas.json", true);
  const m = text === null ? {} : JSON.parse(text);
  return (file, pixel) => (pixel ? m.images : m.smooth)?.[joinPath("", file)] ?? [file, undefined];
}

function sheetArgs(name, sheet, atlas, pixel) {
  if (!sheet || typeof sheet.image !== "string") fail(`assets.sheets.${name}.image must be an image file path`);
  const anims = sheet.animations ?? {};
  if (typeof anims !== "object") fail(`assets.sheets.${name}.animations must be { name: { frames, fps?, loop? } }`);
  return [name, ...atlas(sheet.image, pixel), sheet.frame, Object.entries(anims)];
}

// obj.pixelMode (true = crisp nearest filtering, false = smooth linear), or `fallback`.
function pixelOf(obj, what, fallback) {
  const p = obj?.pixelMode;
  if (p === undefined) return fallback;
  if (typeof p !== "boolean") fail(`${what}.pixelMode must be true (crisp pixel art) or false (smooth), got ${show(p)}`);
  return p;
}

// An asset entry: "file" or { file, pixelMode? } -> { file, pixelMode }.
function fileEntry(entry, what, fallback) {
  if (typeof entry === "string") return { file: entry, pixelMode: fallback };
  if (!entry || typeof entry !== "object" || typeof entry.file !== "string") {
    fail(`${what} must be a file path or { file, pixelMode? }, got ${show(entry)}`);
  }
  return { file: entry.file, pixelMode: pixelOf(entry, what, fallback) };
}

// Every asset is loaded (or synthesized) before the first scene starts.
function preload(s, assets, pixel) {
  for (const [name, sound] of Object.entries(assets.sounds ?? {})) {
    if (typeof sound === "string") N.loadSound(name, sound);
    else N.synthSound(name, sound);
  }
  const atlas = atlasOf();
  const images = Object.entries(assets.images ?? {}).map(([name, e]) => [name, fileEntry(e, `assets.images.${name}`, pixel)]);
  for (const [name, e] of images) N.loadImage(name, ...atlas(e.file, e.pixelMode), e.pixelMode);
  for (const [name, sheet] of Object.entries(assets.sheets ?? {})) {
    const p = pixelOf(sheet, `assets.sheets.${name}`, pixel);
    N.loadSheet(...sheetArgs(name, sheet, atlas, p), p);
  }
  s.maps = new Map(Object.entries(assets.maps ?? {}).map(([name, m]) => {
    const e = fileEntry(m, `assets.maps.${name}`, pixel);
    const map = loadMap(name, e.file);
    map.pixelMode = e.pixelMode;  // its tilesets filter like this
    return [name, map];
  }));
  for (const [name, f] of Object.entries(assets.fonts ?? {})) {
    const e = fileEntry(f, `assets.fonts.${name}`, pixel);
    N.loadFont(name, e.file, e.pixelMode);
  }
  for (const [name, file] of Object.entries(assets.ui ?? {})) N.loadUi(name, file);
}

// ctx.ui errors start with the call, e.g. `ui.set("hud", "scroe"): no element ...`.
function uiCall(what, args, fn) {
  try {
    return fn();
  } catch (e) {
    fail(`ui.${what}(${args.map((a) => JSON.stringify(a)).join(", ")}): ${e.message}`);
  }
}

// ctx.save backends: the engine's save file, or memory only (test runs).
export const fileSave = {
  read: (title) => JSON.parse(N.saveRead(title) || "{}"),
  write: (data) => N.saveWrite(JSON.stringify(data)),
};

export function memorySave() {
  return { read: () => ({}), write: () => {} };
}

function makeEntities() {
  const data = new Map();  // id -> entity.data
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
      get animation() { return N.animation(id); },
      set animation(name) { N.play(id, name, false); },
      play: (name) => N.play(id, name, true),
      get tint() { return N.look(id)[0]; },
      set tint(v) { const l = N.look(id); N.setLook(id, v, l[1], l[2]); },
      get alpha() { return N.look(id)[1]; },
      set alpha(v) { const l = N.look(id); N.setLook(id, l[0], v, l[2]); },
      get blend() { return N.look(id)[2]; },
      set blend(v) { const l = N.look(id); N.setLook(id, l[0], l[1], v); },
      get text() { return N.text(id); },
      set text(v) { N.text(id, String(v)); },
      get layer() { return N.layer(id); },
      emit: (count) => N.emit(id, count),
      get emitting() { return N.emitting(id); },
      set emitting(v) { N.emitting(id, Boolean(v)); },
      get particles() { return N.particles(id); },
      get flipX() { return N.flip(id)[0]; },
      set flipX(v) { N.flip(id, Boolean(v), N.flip(id)[1]); },
      get flipY() { return N.flip(id)[1]; },
      set flipY(v) { N.flip(id, N.flip(id)[0], Boolean(v)); },
      body: bodyOf(id),
      get scale() { return N.scale(id); },
      set scale(v) { N.scale(id, v); },
      get parent() {
        const p = N.parent(id);
        return p === null ? null : entity(p);
      },
      get children() { return N.children(id).map(entity); },
      get data() {
        if (!data.has(id)) data.set(id, {});
        return data.get(id);
      },
      set data(v) { data.set(id, checkData(v, `entity ${id}: data`)); },
    };
    entities.set(id, e);
    return e;
  }
  return { entity, data, clear: () => { entities.clear(); data.clear(); } };
}

// entity.data: a JSON-safe plain object, copied so prefabs never share one.
function checkData(v, what) {
  if (v === null || typeof v !== "object" || Array.isArray(v) || !jsonSafe(v)) {
    fail(`${what} must be a JSON-safe plain object (numbers, strings, booleans, null, arrays, objects), got ${show(v)}`);
  }
  return JSON.parse(JSON.stringify(v));
}

// Mirrors every living entity's data into the engine (MCP state, dev tools).
function syncData(data) {
  const ids = [];
  const texts = [];
  for (const [id, obj] of data) {
    if (!N.alive(id)) data.delete(id);
    else if (!jsonSafe(obj)) fail(`entity ${id}: data must stay JSON-safe (numbers, strings, booleans, null, arrays, plain objects); got ${show(obj)}`);
    else {
      ids.push(id);
      texts.push(JSON.stringify(obj));
    }
  }
  if (ids.length) N.setData(ids, texts);
}

// spawn({ ..., parent?, data? }): with a parent, pos and rotation are local to it.
function spawnWith(s, spec) {
  const parent = spec?.parent;
  if (parent !== undefined && (parent === null || typeof parent.id !== "number")) fail("spawn: parent must be an entity (from ctx.spawn / find)");
  const data = spec?.data === undefined ? undefined : checkData(spec.data, "spawn: data");
  const id = N.spawn(spec);
  if (parent !== undefined) N.adopt(id, parent.id, spec.pos[0], spec.pos[1]);
  if (data !== undefined) s.entities.data.set(id, data);
  return s.entities.entity(id);
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

// ctx.input for player p (1..4), or any player (0).
function playerInput(p) {
  return {
    pressed: (action) => N.action(action, 0, p),
    down: (action) => N.action(action, 1, p),
    released: (action) => N.action(action, 2, p),
    axis: (name) => N.axis(name, p),
  };
}

function checkPlayer(n, what) {
  if (!Number.isInteger(n) || n < 1 || n > 4) fail(`${what}: player must be an integer 1..4, got ${n}`);
  return n;
}

function makeInput() {
  const players = [1, 2, 3, 4].map(playerInput);
  return {
    ...playerInput(0),
    mouse: () => N.mouse(),
    player: (n) => players[checkPlayer(n, `input.player(${n})`) - 1],
    players: () => N.players(),
    rumble: (strength, seconds, player) =>
      N.rumble(strength, seconds, player === undefined ? 0 : checkPlayer(player, "input.rumble")),
  };
}

function pair(v, what) {
  if (!Array.isArray(v) || v.length !== 2) fail(`${what} must be [x, y], got ${JSON.stringify(v)}`);
  return v;
}

// ctx.camera: the current scene's camera (each scene starts with the identity one).
function makeCamera() {
  const get = () => N.cameraGet();
  const set = (i, v) => {
    const c = get();
    c[i] = v;
    N.cameraSet(...c);
  };
  return {
    pos: {
      get x() { return get()[0]; },
      set x(v) { set(0, v); },
      get y() { return get()[1]; },
      set y(v) { set(1, v); },
    },
    get zoom() { return get()[2]; },
    set zoom(v) { set(2, v); },
    get rotation() { return get()[3]; },
    set rotation(v) { set(3, v); },
    follow(entity, opts = {}) {
      if (entity === null) return N.cameraFollow(null, 1, 0, 0);
      if (!entity || typeof entity.id !== "number") fail("camera.follow(entity, opts?): entity must be an entity from ctx.spawn/find, or null to stop");
      const dz = pair(opts.deadzone ?? [0, 0], "camera.follow deadzone");
      N.cameraFollow(entity.id, opts.lerp ?? 1, dz[0], dz[1]);
    },
    shake: (intensity, seconds) => N.cameraShake(intensity, seconds),
    bounds(rect) {
      if (rect === null) return N.cameraBounds(null, 0, 0, 0);
      if (!Array.isArray(rect) || rect.length !== 4) fail(`camera.bounds must be [x, y, width, height] or null, got ${JSON.stringify(rect)}`);
      N.cameraBounds(...rect);
    },
    toWorld: (p) => N.toWorld(...pair(p, "camera.toWorld point")),
  };
}

function mapOf(s, name, what) {
  const map = s.maps.get(name);
  if (!map) fail(`${what}("${name}"): not in assets.maps; have: ${[...s.maps.keys()].join(", ") || "(none)"}`);
  return map;
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
    input: makeInput(),
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
      show: (name) => uiCall("show", [name], () => N.uiShow(name)),
      hide: (name) => uiCall("hide", [name], () => N.uiHide(name)),
      set: (name, id, text) => uiCall("set", [name, id], () => N.uiSet(name, id, String(text))),
      on(name, id, event, fn) {
        if (typeof fn !== "function") fail(`ui.on("${name}", "${id}", "${event}", fn): fn must be a function`);
        s.uiHandlers.set(uiCall("on", [name, id, event], () => N.uiOn(name, id, event)), fn);
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
    camera: makeCamera(),
    window: {
      get fullscreen() { return N.windowGet()[0]; },
      set fullscreen(v) { N.windowFullscreen(Boolean(v)); },
      get size() { const w = N.windowGet(); return [w[1], w[2]]; },
    },
    spawn: (spec) => spawnWith(s, spec),
    ...physicsCtx((id) => s.entities.entity(id)),
    tween: (target, opts) => s.tweens.add(target, opts, aliveOf(target)),
    map: (name) => mapView(mapOf(s, name, "map")),
    spawnMap(name, opts = {}) {
      const map = mapOf(s, name, "spawnMap");
      spawnLayers(map, opts, (spec) => N.spawn(spec));
      return mapView(map);
    },
    destroy: (e) => N.destroy(e.id),
    find(tag) {
      const ids = N.findAll(tag);
      if (!ids.length) fail(`find("${tag}"): no living entity has this tag`);
      return s.entities.entity(ids[0]);
    },
    findAll: (tag) => N.findAll(tag).map(s.entities.entity),
    onHit: (tagA, tagB, fn) => s.handlers.push({ kind: "hits", tagA, tagB, fn }),
    onEnter: (tagA, tagB, fn) => s.handlers.push({ kind: "enters", tagA, tagB, fn }),
    onHitEnd: (tagA, tagB, fn) => s.handlers.push({ kind: "hitEnds", tagA, tagB, fn }),
    onExit: (tagA, tagB, fn) => s.handlers.push({ kind: "exits", tagA, tagB, fn }),
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

// `kept`: {state} carried over from the same scene of a previous run of the
// game (see carried); its state replaces state(params) only if the scene
// still declares state.
function enter(s, ctx, name, params, kept) {
  s.current?.def.exit?.(ctx);
  N.resetWorld();
  N.clearTimers();
  s.timers.clear();
  s.entities.clear();
  s.handlers = [];
  s.tweens.clear();
  N.uiReset();
  s.uiHandlers.clear();
  s.sceneTime = 0;
  s.current = s.game.scenes[name];
  s.currentName = name;
  s.currentParams = params;
  const def = s.current.def;
  if (!def.state) ctx.state = undefined;
  else ctx.state = kept && Object.hasOwn(kept, "state") ? kept.state : def.state(params);
  s.current.def.enter?.(ctx, params);
}

// Calls handlers of `kind` for each [a, b, ...] row, first entity having tagA;
// hit rows also carry the contact (seen from the handler's first entity).
function dispatch(s, rows, kind) {
  for (const row of rows) {
    const [a, b] = row;
    for (const h of s.handlers) {
      if (h.kind !== kind || !N.alive(a) || !N.alive(b)) continue;
      const contact = (swapped) => (kind === "hits" ? contactOf(row, swapped) : undefined);
      if (N.hasTag(a, h.tagA) && N.hasTag(b, h.tagB)) h.fn(s.entities.entity(a), s.entities.entity(b), contact(false));
      else if (N.hasTag(b, h.tagA) && N.hasTag(a, h.tagB)) h.fn(s.entities.entity(b), s.entities.entity(a), contact(true));
    }
  }
}

// True for values that survive JSON.stringify + JSON.parse unchanged: null,
// booleans, finite numbers, strings, arrays and plain objects of those.
function jsonSafe(v, depth = 0) {
  if (depth > 64) return false;
  if (v === null || typeof v === "string" || typeof v === "boolean") return true;
  if (typeof v === "number") return Number.isFinite(v);
  if (Array.isArray(v)) return v.every((x) => jsonSafe(x, depth + 1));
  if (typeof v !== "object") return false;
  const proto = Object.getPrototypeOf(v);
  if (proto !== Object.prototype && proto !== null) return false;
  return Object.values(v).every((x) => jsonSafe(x, depth + 1));
}

// The running scene as JSON text another run of the game can start from:
// {scene, params, state?}; state only when it is JSON-safe. "" when there is
// no scene or its params are not JSON-safe (then the next run starts fresh).
function carried(s, ctx) {
  const params = s.currentParams;
  if (!s.currentName || (params !== undefined && !jsonSafe(params))) return "";
  const out = { scene: s.currentName, params };
  if (ctx.state !== undefined && jsonSafe(ctx.state)) out.state = ctx.state;
  return JSON.stringify(out);
}

// Where boot starts: the carried scene if the game still has it, else `start`.
function startOf(s, start, what, from) {
  if (from && Object.hasOwn(s.game.scenes, from.scene)) return from;
  return { scene: start === undefined ? s.game.start : checkScene(s.game, start, what) };
}

function bootGame(s, ctx, g, start, what, from) {
  s.game = validate(g);
  const c = s.game.config;
  N.configure(c.seed ?? 0, ...(c.gravity ?? [0, 0]), c.size[0], c.size[1]);
  N.uiCreate(c.size[0], c.size[1]);
  N.collisionCategories(collisionCategories(c.collision));
  for (const [action, inputs] of Object.entries(c.actions ?? {})) N.bind("actions", action, inputs);
  if (c.deadzone !== undefined) N.deadzone(c.deadzone);
  for (const [axis, sources] of Object.entries(c.axes ?? {})) N.bind("axes", axis, sources);
  defineLayers(c.layers ?? {});
  preload(s, s.game.assets ?? {}, pixelOf(c, "config", true));
  s.saveData = s.save.read(c.title ?? "alpha-star");
  const at = startOf(s, start, what, from);
  enter(s, ctx, at.scene, at.params, at);
  return { title: c.title ?? "alpha-star", size: c.size, background: c.background ?? "#000000", window: c.window };
}

// config.collision: { categories: [names] } for body.category / collidesWith and query filters.
function collisionCategories(collision) {
  if (collision === undefined) return [];
  if (!collision || typeof collision !== "object" || !Array.isArray(collision.categories)) {
    fail(`config.collision must be { categories: ["player", "enemy", ...] }, got ${show(collision)}`);
  }
  return collision.categories;
}

// config.layers: { name: { z, parallax? } }, drawn in z order (then entity z).
function defineLayers(layers) {
  if (typeof layers !== "object") fail("config.layers must be { name: { z, parallax? } }");
  for (const [name, l] of Object.entries(layers)) {
    if (!l || typeof l.z !== "number") fail(`config.layers.${name} must be { z: number, parallax?: number }, got ${JSON.stringify(l)}`);
    N.defineLayer(name, l.z, l.parallax ?? 1);
  }
}

function stepGame(s, ctx, dt) {
  s.stepDt = dt;
  for (const id of N.uiEvents()) s.uiHandlers.get(id)?.();  // clicks since the last step
  s.current.def.update?.(ctx);
  for (const id of N.advanceTimers(dt)) s.timers.get(id)?.();
  s.tweens.step(dt);
  const events = N.stepWorld(dt);
  s.sceneTime += dt;
  dispatch(s, events.hits, "hits");
  dispatch(s, events.enters, "enters");
  dispatch(s, events.hitEnds, "hitEnds");
  dispatch(s, events.exits, "exits");
  syncData(s.entities.data);
  if (s.pending) {
    const { name, params } = s.pending;
    s.pending = null;
    enter(s, ctx, name, params);
  }
}

function snapshotOf(e) {
  const out = { id: e.id, pos: [e.pos.x, e.pos.y], rotation: e.rotation, scale: 1, alpha: 1, data: JSON.parse(JSON.stringify(e.data)) };
  try {
    out.alpha = e.alpha;
    out.scale = e.scale;
  } catch {
    // no shape: alpha and scale stay 1
  }
  out.velocity = N.bodyType(e.id) === null ? null : [e.body.velocity.x, e.body.velocity.y];
  return out;
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
    game: null, current: null, currentName: null, currentParams: undefined, pending: null, handlers: [], uiHandlers: new Map(),
    saveData: {}, save, sceneTime: 0, stepDt: 1 / 60, masterVolume: 1,
    timers: new Map(), entities: makeEntities(), maps: new Map(), tweens: makeTweens(tweenAccess),
  };
  const ctx = makeCtx(s);
  return {
    // from: carried() of an earlier run of this game, parsed, or undefined.
    boot: (g, start, what, from) => within(id, () => bootGame(s, ctx, g, start, what, from)),
    carried: () => carried(s, ctx),
    step: (dt) => within(id, () => stepGame(s, ctx, dt)),
    // Runs fn with this instance active (for the headless test runner).
    within: (fn) => within(id, fn),
    actions: () => Object.keys(s.game?.config.actions ?? {}),
    axes: () => Object.keys(s.game?.config.axes ?? {}),
    scene: () => s.currentName,
    // For headless test runs: living entities with `tag` (oldest first) and the scene state, as plain data.
    entities: (tag) => within(id, () => N.findAll(tag).map((e) => snapshotOf(s.entities.entity(e)))),
    state: () => (jsonSafe(ctx.state) ? JSON.parse(JSON.stringify(ctx.state ?? null)) : undefined),
  };
}

// ---- host entry points (the engine's own game, instance 0) -------------------

const main = createInstance(0, fileSave);

// from: JSON text from carried() of the run this one replaces, or "".
export function boot(g, from) {
  return main.boot(g, undefined, undefined, from ? JSON.parse(from) : undefined);
}

export function carriedScene() {
  return main.carried();
}

export function step(dt) {
  main.step(dt);
}

export function sceneName() {
  return main.scene();
}
