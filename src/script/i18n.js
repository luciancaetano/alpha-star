// Localization: assets.locales { en: "locales/en.json", ... } and ctx.t(key, params?).
// Embedded module "alpha:i18n". A locale file is a JSON object of texts, nested
// objects allowed ("menu.play" finds { menu: { play } }). A text may hold
// {name} placeholders, filled from params, and plural forms as an object
// { zero?, one, other } picked by params.count. A key missing in the current
// locale falls back to config.locale's text, else shows "[key]" (and warns
// once in debug-tools builds).
const N = globalThis.__native;

function fail(message) {
  throw new Error(message);
}

function lookup(table, key) {
  if (Object.hasOwn(table, key)) return table[key];
  let v = table;
  for (const part of key.split(".")) {
    if (!v || typeof v !== "object" || !Object.hasOwn(v, part)) return undefined;
    v = v[part];
  }
  return v;
}

function plural(value, params) {
  if (!value || typeof value !== "object") return value;
  const n = params?.count;
  if (n === 0 && typeof value.zero === "string") return value.zero;
  return n === 1 ? value.one ?? value.other : value.other ?? value.one;
}

function fill(text, params) {
  return text.replace(/\{(\w+)\}/g, (whole, name) => (params && Object.hasOwn(params, name) ? String(params[name]) : whole));
}

function parseLocale(name, file) {
  const text = N.readText(file, false);
  let table;
  try {
    table = JSON.parse(text);
  } catch (e) {
    fail(`assets.locales.${name} ("${file}") is not valid JSON: ${e.message}`);
  }
  if (!table || typeof table !== "object" || Array.isArray(table)) fail(`assets.locales.${name} ("${file}") must hold a JSON object of texts`);
  return table;
}

export function makeI18n() {
  const tables = new Map();
  const warned = new Set();
  let fallback = null;
  let current = null;
  const check = (name, what) => {
    if (!tables.has(name)) fail(`${what}: "${name}" is not in assets.locales; locales: ${[...tables.keys()].join(", ") || "(none)"}`);
    return name;
  };
  return {
    // configLocale: config.locale, the default and fallback (else the first locale).
    load(locales, configLocale) {
      for (const [name, file] of Object.entries(locales ?? {})) tables.set(name, parseLocale(name, file));
      if (!tables.size) return;
      fallback = configLocale === undefined ? tables.keys().next().value : check(configLocale, "config.locale");
      current = fallback;
    },
    get locale() { return current; },
    get locales() { return [...tables.keys()]; },
    has: (name) => tables.has(name),
    set(name) {
      current = check(name, "ctx.locale");
    },
    t(key, params) {
      if (typeof key !== "string") fail(`t(key): key must be a string, got ${JSON.stringify(key)}`);
      const raw = (current && lookup(tables.get(current), key)) ?? (fallback && lookup(tables.get(fallback), key));
      const text = plural(raw, params);
      if (typeof text === "string") return fill(text, params);
      if (!warned.has(`${current}:${key}`)) {
        warned.add(`${current}:${key}`);
        N.devWarn(`t("${key}"): missing in locale "${current ?? "(none: add assets.locales)"}"`);
      }
      return `[${key}]`;
    },
  };
}
