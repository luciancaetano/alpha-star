// What a game keeps between runs: ctx.save (slots, versioned, migrated) and
// ctx.settings (volumes, locale, fullscreen, control bindings, accessibility).
// Embedded module "alpha:prefs". Files are written atomically by the engine
// (temp file + rename, previous file kept as .bak); a file that does not parse
// falls back to its .bak with a logged warning.
const N = globalThis.__native;

function fail(message) {
  throw new Error(message);
}

// True for values that survive JSON.stringify + JSON.parse unchanged: null,
// booleans, finite numbers, strings, arrays and plain objects of those.
export function jsonSafe(v, depth = 0) {
  if (depth > 64) return false;
  if (v === null || typeof v === "string" || typeof v === "boolean") return true;
  if (typeof v === "number") return Number.isFinite(v);
  if (Array.isArray(v)) return v.every((x) => jsonSafe(x, depth + 1));
  if (typeof v !== "object") return false;
  const proto = Object.getPrototypeOf(v);
  if (proto !== Object.prototype && proto !== null) return false;
  return Object.values(v).every((x) => jsonSafe(x, depth + 1));
}

// Stores: read(name, backup) -> text or null; write(name, text).
export function fileStore() {
  let title = "alpha-star";
  return {
    open: (t) => { title = t; },
    read: (name, backup) => N.prefsRead(title, name, backup),
    write: (name, text) => N.prefsWrite(title, name, text),
  };
}

// In memory (test runs): `files` seeds { name: text }; nothing touches the disk.
export function memoryStore(files = {}) {
  const m = new Map(Object.entries(files));
  return {
    open: () => {},
    read: (name, backup) => (backup ? null : m.get(name) ?? null),
    write: (name, text) => m.set(name, text),
    files: () => Object.fromEntries(m),
  };
}

// Parsed file, or its .bak when it does not parse; null when neither exists.
function readJson(store, name) {
  const parse = (text, which) => {
    if (text === null) return null;
    try {
      return JSON.parse(text);
    } catch {
      console.warn(`${which} is not valid JSON`);
      return undefined;
    }
  };
  const main = parse(store.read(name, false), `${name}.json`);
  if (main !== undefined) return main;
  const backup = parse(store.read(name, true), `${name}.json.bak`);
  console.warn(backup ? `${name}.json: restored from ${name}.json.bak` : `${name}.json: no usable backup; starting empty`);
  return backup ?? null;
}

// config.save: { version?: positive integer, migrate?(data, fromVersion) }.
export function saveConfig(c) {
  if (c === undefined) return { version: 1, migrate: undefined };
  const ok = c && typeof c === "object" && (c.version === undefined || (Number.isInteger(c.version) && c.version >= 1)) &&
    (c.migrate === undefined || typeof c.migrate === "function");
  if (!ok) fail("config.save must be { version?: integer >= 1, migrate?(data, fromVersion) { return newData } }");
  return { version: c.version ?? 1, migrate: c.migrate };
}

// A stored slot { version, data } (files from before versioning: plain data = version 1)
// brought to config.save.version.
function migrated(raw, cfg, what) {
  const wrapped = raw && typeof raw.version === "number" && raw.data && typeof raw.data === "object";
  const from = wrapped ? raw.version : 1;
  const data = wrapped ? raw.data : raw;
  if (from === cfg.version) return data;
  if (from > cfg.version) {
    console.warn(`${what} is version ${from}, newer than config.save.version ${cfg.version}; loaded as is`);
    return data;
  }
  if (!cfg.migrate) fail(`${what} is version ${from} but config.save.version is ${cfg.version}: add config.save.migrate(data, fromVersion) returning the new data`);
  const out = cfg.migrate(data, from);
  if (!out || typeof out !== "object" || Array.isArray(out) || !jsonSafe(out)) {
    fail(`config.save.migrate(data, ${from}) must return a JSON-safe plain object, got ${JSON.stringify(out) ?? String(out)}`);
  }
  return out;
}

const SLOT_NAME = /^[A-Za-z0-9_-]{1,32}$/;

// One save slot: lazily read, every change written whole.
function makeSlot(store, file, cfg, what) {
  let data;
  const load = () => {
    if (data === undefined) {
      const raw = readJson(store, file);
      data = raw && typeof raw === "object" ? migrated(raw, cfg, what) : {};
      if (raw && typeof raw === "object" && (raw.version ?? 1) !== cfg.version) write();
    }
    return data;
  };
  const write = () => store.write(file, JSON.stringify({ version: cfg.version, data }));
  return {
    get: (key, fallback) => (Object.hasOwn(load(), key) ? load()[key] : fallback),
    set(key, value) {
      if (!jsonSafe(value)) fail(`${what}.set("${key}", ...): value must be JSON-safe (numbers, strings, booleans, null, arrays, plain objects), got ${String(value)}`);
      load()[key] = JSON.parse(JSON.stringify(value));
      write();
    },
    has: (key) => Object.hasOwn(load(), key),
    remove(key) {
      delete load()[key];
      write();
    },
    clear() {
      data = {};
      write();
    },
    keys: () => Object.keys(load()),
  };
}

// ctx.save: the "main" slot (file save.json) plus slot(name) (save-<name>.json).
export function makeSave(store, cfg) {
  const slots = new Map();
  const slot = (name) => {
    if (typeof name !== "string" || !SLOT_NAME.test(name)) fail(`save.slot(${JSON.stringify(name)}): slot names are 1..32 letters, digits, '-' or '_'`);
    if (!slots.has(name)) slots.set(name, makeSlot(store, name === "main" ? "save" : `save-${name}`, cfg, name === "main" ? "save" : `save.slot("${name}")`));
    return slots.get(name);
  };
  const main = slot("main");
  return { ...main, slot };
}

export const SETTINGS_DEFAULTS = Object.freeze({
  volumes: {}, locale: null, fullscreen: null, bindings: {}, uiScale: 1, highContrast: false, reducedMotion: false,
});

// The persisted settings object: defaults overlaid with settings.json.
export function readSettings(store) {
  const raw = readJson(store, "settings");
  const s = { ...SETTINGS_DEFAULTS, volumes: {}, bindings: {} };
  if (!raw || typeof raw !== "object") return s;
  for (const key of Object.keys(SETTINGS_DEFAULTS)) {
    if (Object.hasOwn(raw, key)) s[key] = raw[key];
  }
  return s;
}

export function writeSettings(store, s) {
  store.write("settings", JSON.stringify(s));
}
