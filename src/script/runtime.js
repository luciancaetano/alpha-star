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
import { defineBuses, loadAudio, makeAudio } from "alpha:audio";
import { makeI18n } from "alpha:i18n";
import { SETTINGS_DEFAULTS, fileStore, jsonSafe, makeSave, readSettings, saveConfig, writeSettings } from "alpha:prefs";

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
  loadAudio(assets);
  N.shapePixelMode(pixel);
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
    if (f?.license !== undefined && typeof f.license !== "string") fail(`assets.fonts.${name}.license must be a file path (the font's license, credited by alpha export), got ${show(f.license)}`);
    N.loadFont(name, e.file, e.pixelMode);
  }
  for (const [name, file] of Object.entries(assets.ui ?? {})) N.loadUi(name, file);
  s.i18n.load(assets.locales, s.game.config.locale);
  s.defaultLocale = s.i18n.locale;
}

// ctx.ui errors start with the call, e.g. `ui.set("hud", "scroe"): no element ...`.
function uiCall(what, args, fn) {
  try {
    return fn();
  } catch (e) {
    fail(`ui.${what}(${args.map((a) => JSON.stringify(a)).join(", ")}): ${e.message}`);
  }
}

// Entity proxies share their accessors through prototypes: an object literal
// of closures per entity cost ~7 KB of JS heap each (10000 entities = 70 MB).
const posProto = {
  get x() { return N.position(this.id)[0]; },
  set x(v) { N.setPosition(this.id, v, N.position(this.id)[1]); },
  get y() { return N.position(this.id)[1]; },
  set y(v) { N.setPosition(this.id, N.position(this.id)[0], v); },
};

function entityProto(entity, data) {
  return {
    get alive() { return N.alive(this.id); },
    get tags() { return N.tags(this.id); },
    get rotation() { return N.rotation(this.id); },
    set rotation(v) { N.setRotation(this.id, v); },
    get z() { return N.z(this.id); },
    set z(v) { N.setZ(this.id, v); },
    get animation() { return N.animation(this.id); },
    set animation(name) { N.play(this.id, name, false); },
    play(name) { return N.play(this.id, name, true); },
    get tint() { return N.look(this.id)[0]; },
    set tint(v) { const l = N.look(this.id); N.setLook(this.id, v, l[1], l[2]); },
    get alpha() { return N.look(this.id)[1]; },
    set alpha(v) { const l = N.look(this.id); N.setLook(this.id, l[0], v, l[2]); },
    get blend() { return N.look(this.id)[2]; },
    set blend(v) { const l = N.look(this.id); N.setLook(this.id, l[0], l[1], v); },
    get text() { return N.text(this.id); },
    set text(v) { N.text(this.id, String(v)); },
    get layer() { return N.layer(this.id); },
    emit(count) { return N.emit(this.id, count); },
    get emitting() { return N.emitting(this.id); },
    set emitting(v) { N.emitting(this.id, Boolean(v)); },
    get particles() { return N.particles(this.id); },
    get flipX() { return N.flip(this.id)[0]; },
    set flipX(v) { N.flip(this.id, Boolean(v), N.flip(this.id)[1]); },
    get flipY() { return N.flip(this.id)[1]; },
    set flipY(v) { N.flip(this.id, N.flip(this.id)[0], Boolean(v)); },
    get body() {  // made on first use, then kept (same object every time)
      const body = bodyOf(this.id);
      Object.defineProperty(this, "body", { value: body, enumerable: true });
      return body;
    },
    get scale() { return N.scale(this.id); },
    set scale(v) { N.scale(this.id, v); },
    get parent() {
      const p = N.parent(this.id);
      return p === null ? null : entity(p);
    },
    get children() { return N.children(this.id).map(entity); },
    get data() {
      if (!data.has(this.id)) data.set(this.id, {});
      return data.get(this.id);
    },
    set data(v) { data.set(this.id, checkData(v, `entity ${this.id}: data`)); },
  };
}

function makeEntities() {
  const data = new Map();  // id -> entity.data
  const entities = new Map();
  const proto = entityProto(entity, data);
  function entity(id) {
    let e = entities.get(id);
    if (e) return e;
    e = Object.create(proto);
    e.id = id;
    e.pos = Object.create(posProto, { id: { value: id } });
    entities.set(id, e);
    return e;
  }
  // Freed entities leave both maps (a script may still hold a proxy: it reads as dead).
  function forget(ids) {
    for (const id of ids) {
      entities.delete(id);
      data.delete(id);
    }
  }
  return { entity, data, forget, clear: () => { entities.clear(); data.clear(); } };
}

// ctx.positions(tag): every living entity with the tag in two native calls.
// x[i], y[i] are entity ids[i]'s position; write() sets them all back.
function positionsOf(tag) {
  const buf = N.positionsOf(tag);
  const count = buf.byteLength / 24;
  return {
    count,
    ids: new Float64Array(buf, 0, count),
    x: new Float64Array(buf, count * 8, count),
    y: new Float64Array(buf, count * 16, count),
    write: () => N.setPositions(buf),
  };
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

function checkAction(s, action, what) {
  const actions = s.game.config.actions ?? {};
  if (!Object.hasOwn(actions, action)) fail(`${what}("${action}"): not in config.actions; actions: ${Object.keys(actions).join(", ") || "(none)"}`);
  return action;
}

// rebind: the player's own inputs for an action, persisted in settings (bindings).
function makeInput(s) {
  const players = [1, 2, 3, 4].map(playerInput);
  return {
    ...playerInput(0),
    rebind(action, inputs) {
      checkAction(s, action, "input.rebind");
      try {
        N.bind("actions", action, inputs);
      } catch (e) {
        fail(`input.rebind("${action}", ${JSON.stringify(inputs)}): ${e.message}`);
      }
      s.settings.bindings[action] = [...inputs];
      persistSettings(s);
    },
    bindings: (action) => [...(s.settings.bindings[checkAction(s, action, "input.bindings")] ?? s.game.config.actions[action])],
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

// ctx.postfx: the current scene's full-screen effects, drawn in the order first added.
function makePostfx() {
  return {
    add: (name, params) => N.postfxPut(name, params),
    set: (name, params) => {
      if (!N.postfxList().includes(name)) fail(`postfx.set("${name}"): not added in this scene; have: ${N.postfxList().join(" ") || "(none)"}; use postfx.add`);
      N.postfxPut(name, params);
    },
    remove: (name) => N.postfxRemove(name),
    clear: () => N.postfxClear(),
    list: () => N.postfxList(),
  };
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

function persistSettings(s) {
  writeSettings(s.store, s.settings);
}

// data-t elements of the game's documents show ctx.t(key) in the current locale.
function syncLocale(s) {
  N.localeSet(s.i18n.locale ?? "");
  const keys = N.uiKeys();
  if (keys.length) N.uiTranslate(keys, keys.map((k) => s.i18n.t(k)));
}

function applyAccess(s) {
  const st = s.settings;
  N.uiAccess(st.uiScale, st.highContrast);
  N.reducedMotion(st.reducedMotion);
}

// Settings from the player's file are applied leniently: a value this game
// cannot use (a bus or locale it no longer has) is skipped with a warning.
function applySettings(s) {
  const st = s.settings;
  const skip = (what, e) => console.warn(`settings: ${what} ignored (${e.message})`);
  for (const [bus, v] of Object.entries(st.volumes ?? {})) {
    try { N.busVolume(bus, v); } catch (e) { skip(`volumes.${bus}`, e); }
  }
  for (const [action, inputs] of Object.entries(st.bindings ?? {})) {
    try { N.bind("actions", checkAction(s, action, "settings.bindings"), inputs); } catch (e) { skip(`bindings.${action}`, e); }
  }
  if (typeof st.locale === "string") {
    try { s.i18n.set(st.locale); } catch (e) { skip("locale", e); }
  }
  if (typeof st.fullscreen === "boolean") N.windowFullscreen(st.fullscreen);
  try { applyAccess(s); } catch (e) {
    skip("uiScale/highContrast/reducedMotion", e);
    Object.assign(st, { uiScale: 1, highContrast: false, reducedMotion: false });
    applyAccess(s);
  }
  syncLocale(s);
}

function flagSetting(s, key) {
  return {
    get: () => s.settings[key],
    set(v) {
      if (typeof v !== "boolean") fail(`settings.${key} must be true or false, got ${show(v)}`);
      s.settings[key] = v;
      applyAccess(s);
      persistSettings(s);
    },
  };
}

// ctx.settings: accessibility flags set here; volumes, locale, fullscreen and
// bindings are read here and set where they live (audio.bus, ctx.locale,
// window.fullscreen, input.rebind), which persists them too.
function makeSettings(s) {
  const api = {
    get uiScale() { return s.settings.uiScale; },
    set uiScale(v) {
      if (typeof v !== "number" || !(v >= 0.5 && v <= 3)) fail(`settings.uiScale must be a number in [0.5, 3], got ${show(v)}`);
      s.settings.uiScale = v;
      applyAccess(s);
      persistSettings(s);
    },
    get volumes() { return Object.fromEntries(N.buses().map((b) => [b, N.busVolume(b)])); },
    get locale() { return s.i18n.locale; },
    get fullscreen() { return N.windowGet()[0]; },
    get bindings() { return JSON.parse(JSON.stringify(s.settings.bindings)); },
    reset() {
      for (const b of N.buses()) N.busVolume(b, 1);
      for (const [action, inputs] of Object.entries(s.game.config.actions ?? {})) N.bind("actions", action, inputs);
      if (s.defaultLocale) s.i18n.set(s.defaultLocale);
      s.settings = { ...SETTINGS_DEFAULTS, volumes: {}, bindings: {} };
      applyAccess(s);
      syncLocale(s);
      persistSettings(s);
    },
  };
  for (const key of ["highContrast", "reducedMotion"]) Object.defineProperty(api, key, { ...flagSetting(s, key), enumerable: true });
  return api;
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
    input: makeInput(s),
    random: {
      range: (min, max) => N.random(min, max, false),
      int: (min, max) => N.random(min, max, true),
      pick: (items) => items[N.random(0, items.length - 1, true)],
    },
    get save() { return s.save; },
    settings: makeSettings(s),
    get locale() { return s.i18n.locale; },
    set locale(name) {
      s.i18n.set(name);
      s.settings.locale = name;
      persistSettings(s);
      syncLocale(s);
    },
    t: (key, params) => s.i18n.t(key, params),
    ui: {
      show: (name) => uiCall("show", [name], () => N.uiShow(name)),
      hide: (name) => uiCall("hide", [name], () => N.uiHide(name)),
      set: (name, id, text) => uiCall("set", [name, id], () => N.uiSet(name, id, String(text))),
      on(name, id, event, fn) {
        if (typeof fn !== "function") fail(`ui.on("${name}", "${id}", "${event}", fn): fn must be a function`);
        s.uiHandlers.set(uiCall("on", [name, id, event], () => N.uiOn(name, id, event)), fn);
      },
    },
    audio: makeAudio((bus, v) => {
      s.settings.volumes[bus] = v;
      persistSettings(s);
    }),
    camera: makeCamera(),
    postfx: makePostfx(),
    window: {
      get fullscreen() { return N.windowGet()[0]; },
      set fullscreen(v) {
        N.windowFullscreen(Boolean(v));
        s.settings.fullscreen = Boolean(v);
        persistSettings(s);
      },
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
    positions: (tag) => positionsOf(tag),
    onHit: (tagA, tagB, fn) => s.handlers.push({ kind: "hits", tagA, tagB, fn }),
    onEnter: (tagA, tagB, fn) => s.handlers.push({ kind: "enters", tagA, tagB, fn }),
    onHitEnd: (tagA, tagB, fn) => s.handlers.push({ kind: "hitEnds", tagA, tagB, fn }),
    onExit: (tagA, tagB, fn) => s.handlers.push({ kind: "exits", tagA, tagB, fn }),
    onPause: appHook(s, "pause", "onPause"),
    onResume: appHook(s, "resume", "onResume"),
    onLowMemory: appHook(s, "lowMemory", "onLowMemory"),
    app: { get paused() { return s.appPaused; } },
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

// ctx.onPause / onResume / onLowMemory(fn): scene-scoped like onHit (cleared on scene change).
function appHook(s, kind, what) {
  return (fn) => {
    if (typeof fn !== "function") fail(`${what}(fn): fn must be a function, got ${show(fn)}`);
    s.handlers.push({ kind, fn });
  };
}

function runHooks(s, ctx, kind) {
  for (const h of s.handlers.filter((x) => x.kind === kind)) h.fn(ctx);
}

// The app paused (focus lost, minimized, background) or resumed; repeated news is ignored.
function appChanged(s, ctx, paused) {
  if (s.appPaused === paused) return;
  s.appPaused = paused;
  runHooks(s, ctx, paused ? "pause" : "resume");
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
  if (c.pauseOnFocusLoss !== undefined && typeof c.pauseOnFocusLoss !== "boolean") {
    fail(`config.pauseOnFocusLoss must be true (pause when the window loses focus or is minimized; the default) or false, got ${show(c.pauseOnFocusLoss)}`);
  }
  N.configure(c.seed ?? 0, ...(c.gravity ?? [0, 0]), c.size[0], c.size[1]);
  N.uiCreate(c.size[0], c.size[1]);
  N.collisionCategories(collisionCategories(c.collision));
  for (const [action, inputs] of Object.entries(c.actions ?? {})) N.bind("actions", action, inputs);
  if (c.deadzone !== undefined) N.deadzone(c.deadzone);
  for (const [axis, sources] of Object.entries(c.axes ?? {})) N.bind("axes", axis, sources);
  defineLayers(c.layers ?? {});
  defineLighting(c.lighting);
  defineBuses(c.audio);
  s.store.open(c.title ?? "alpha-star");
  s.save = makeSave(s.store, saveConfig(c.save));
  preload(s, s.game.assets ?? {}, pixelOf(c, "config", true));
  s.settings = readSettings(s.store);
  applySettings(s);
  const at = startOf(s, start, what, from);
  enter(s, ctx, at.scene, at.params, at);
  return { title: c.title ?? "alpha-star", size: c.size, background: c.background ?? "#000000", window: c.window, pauseOnFocusLoss: c.pauseOnFocusLoss };
}

// config.collision: { categories: [names] } for body.category / collidesWith and query filters.
function collisionCategories(collision) {
  if (collision === undefined) return [];
  if (!collision || typeof collision !== "object" || !Array.isArray(collision.categories)) {
    fail(`config.collision must be { categories: ["player", "enemy", ...] }, got ${show(collision)}`);
  }
  return collision.categories;
}

// config.lighting: { ambient } turns the lightmap on (spawn `light` entities light it up).
function defineLighting(lighting) {
  if (lighting === undefined) return N.setLighting(null);
  if (!lighting || typeof lighting !== "object" || typeof lighting.ambient !== "string") {
    fail(`config.lighting must be { ambient: "#rrggbb" } (the color where no light reaches), got ${show(lighting)}`);
  }
  N.setLighting(lighting.ambient);
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
  const t0 = N.perfNow();
  for (const id of N.advanceTimers(dt)) s.timers.get(id)?.();
  s.tweens.step(dt);
  N.perfAdd(1, N.perfNow() - t0);  // section 1 = timers (with tweens)
  const events = N.stepWorld(dt);
  s.entities.forget(events.freed);
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

// One game instance. `id` is its native instance; `store` keeps its saves and
// settings (fileStore() for the engine's game, memoryStore() for test runs).
export function createInstance(id, store) {
  const s = {
    game: null, current: null, currentName: null, currentParams: undefined, pending: null, handlers: [], uiHandlers: new Map(),
    store, save: null, settings: null, i18n: makeI18n(), defaultLocale: null, sceneTime: 0, stepDt: 1 / 60, appPaused: false,
    timers: new Map(), entities: makeEntities(), maps: new Map(), tweens: makeTweens(tweenAccess),
  };
  const ctx = makeCtx(s);
  return {
    // from: carried() of an earlier run of this game, parsed, or undefined.
    boot: (g, start, what, from) => within(id, () => bootGame(s, ctx, g, start, what, from)),
    carried: () => carried(s, ctx),
    step: (dt) => within(id, () => stepGame(s, ctx, dt)),
    app: (paused) => within(id, () => appChanged(s, ctx, paused)),
    lowMemory: () => within(id, () => runHooks(s, ctx, "lowMemory")),
    // Runs fn with this instance active (for the headless test runner).
    within: (fn) => within(id, fn),
    actions: () => Object.keys(s.game?.config.actions ?? {}),
    axes: () => Object.keys(s.game?.config.axes ?? {}),
    scene: () => s.currentName,
    // For headless test runs: living entities with `tag` (oldest first) and the scene state, as plain data.
    entities: (tag) => within(id, () => N.findAll(tag).map((e) => snapshotOf(s.entities.entity(e)))),
    state: () => (jsonSafe(ctx.state) ? JSON.parse(JSON.stringify(ctx.state ?? null)) : undefined),
    // Audio and settings as plain data (headless test runs).
    music: () => within(id, () => N.musicName()),
    locale: () => s.i18n.locale,
    settings: () => within(id, () => ({ ...JSON.parse(JSON.stringify(s.settings)), volumes: ctx.settings.volumes, locale: s.i18n.locale })),
  };
}

// ---- host entry points (the engine's own game, instance 0) -------------------

const main = createInstance(0, fileStore());

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

export function appPaused(paused) {
  main.app(paused);
}

export function lowMemory() {
  main.lowMemory();
}

export function sceneName() {
  return main.scene();
}
