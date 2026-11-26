// Module "alpha:levels": what scripts do with loaded LDtk worlds. Data views
// (ctx.world) read the parsed project; spawnLevel draws tile layers (one
// batched entity per layer, animated tiles in a second one), turns IntGrid
// values into merged static colliders, spawns entities through factories,
// and returns a live level whose cells can be read and edited. Keep this
// file ASCII (embedded as C++ literals).
import { fail, tileSrc } from "alpha:ldtk";

const N = globalThis.__native;

const clone = (v) => JSON.parse(JSON.stringify(v));

function list(items) {
  return items.join(", ") || "(none)";
}

export function levelOf(world, id, what) {
  const l = world.levels.find((x) => x.id === id);
  if (!l) fail(`${what}: world "${world.name}" has no level "${id}"; it has: ${list(world.levels.map((x) => x.id))}`);
  return l;
}

const KINDS = { ints: "IntGrid", tiles: "tile" };

// Layer `id` of a level view; kind "ints" needs an IntGrid layer, "tiles" one with a tileset.
function layerOf(view, id, kind, what) {
  const fits = (L) => !kind || (kind === "ints" ? L.ints : L.def.tileset);
  const L = view.layers.get(id);
  if (!L || !fits(L)) {
    const have = [...view.layers.values()].filter(fits).map((x) => x.def.id);
    fail(`${what}: level "${view.level.id}" has no ${kind ? KINDS[kind] + " " : ""}layer "${id}"; it has: ${list(have)}`);
  }
  return L;
}

function cellIndex(L, cx, cy, what) {
  if (!Number.isInteger(cx) || !Number.isInteger(cy)) fail(`${what}: cell must be whole numbers, got ${cx}, ${cy}`);
  const [w, h] = L.def.cells;
  if (cx < 0 || cy < 0 || cx >= w || cy >= h) fail(`${what}: cell ${cx}, ${cy} is outside layer "${L.def.id}" (${w} x ${h} cells)`);
  return cy * w + cx;
}

// IntGrid value from a number or its LDtk identifier; 0 clears.
function intValue(L, v, what) {
  const values = L.def.values;
  if (v === 0 || values.has(v)) return v;
  for (const [n, name] of values) if (name !== null && name === v) return n;
  const known = [...values].map(([n, name]) => (name ? `${n} (${name})` : String(n)));
  return fail(`${what}: layer "${L.def.id}" has no IntGrid value ${JSON.stringify(v)}; it has: 0, ${list(known)}`);
}

function worldEntity(view, L, e) {
  const at = [view.pos[0] + L.def.offset[0] + e.local[0], view.pos[1] + L.def.offset[1] + e.local[1]];
  return { ...e, pos: at, layer: L.def.id, local: [...e.local], grid: [...e.grid], size: [...e.size], pivot: [...e.pivot],
    tags: [...e.tags], fields: clone(e.fields), tile: e.tile && { ...e.tile, rect: [...e.tile.rect] } };
}

// ---- drawing -------------------------------------------------------------

// Cells for N.tileLayer: static tiles and animated tiles (at `time`), each [dx, dy, sx, sy, flips, alpha, ...].
function cellsOf(L, time) {
  const ts = L.def.tileset;
  const out = { still: [], anim: [] };
  for (let i = 0; i < L.tiles.length; i += 5) {
    const anim = ts.anims.get(L.tiles[i + 2]);
    const id = anim ? anim.frames[Math.floor(time * anim.fps) % anim.frames.length] : L.tiles[i + 2];
    (anim ? out.anim : out.still).push(L.tiles[i], L.tiles[i + 1], ...tileSrc(ts, id), L.tiles[i + 3], L.tiles[i + 4] * L.def.opacity);
  }
  return out;
}

function animKey(L, time) {
  let key = "";
  for (const a of L.def.tileset.anims.values()) key += Math.floor(time * a.fps) % a.frames.length + ",";
  return key;
}

function drawLayer(rec, L) {
  const ts = L.def.tileset;
  if (!ts || !L.def.visible) return;
  const spec = (cells) => ({ image: ts.image, tile: [ts.grid, ts.grid], cells, z: rec.z, layer: rec.target(L.def.id), filter: rec.world.filter,
    size: [L.def.cells[0] * L.def.grid, L.def.cells[1] * L.def.grid],
    pos: [rec.pos[0] + L.def.offset[0], rec.pos[1] + L.def.offset[1]] });
  const cells = cellsOf(L, rec.time);
  L.drawId = N.tileLayer(spec(cells.still));
  if (ts.anims.size) {
    L.animId = N.tileLayer(spec(cells.anim));
    L.animKey = animKey(L, rec.time);
  }
}

function redraw(rec, L) {
  if (L.drawId === undefined) return;
  const ts = L.def.tileset;
  const cells = cellsOf(L, rec.time);
  N.tileCells(L.drawId, [ts.grid, ts.grid], cells.still);
  if (L.animId !== undefined) N.tileCells(L.animId, [ts.grid, ts.grid], cells.anim);
}

// Advances animated tiles of every spawned level to scene time `time`.
export function stepLevels(recs, time) {
  for (const rec of recs) {
    if (rec.dead) continue;
    rec.time = time - rec.start;
    for (const L of rec.layers.values()) {
      if (L.animId === undefined) continue;
      const key = animKey(L, rec.time);
      if (key === L.animKey) continue;
      L.animKey = key;
      N.tileCells(L.animId, [L.def.tileset.grid, L.def.tileset.grid], cellsOf(L, rec.time).anim);
    }
  }
}

// ---- colliders -----------------------------------------------------------

// collide: { layer: "tag" } (every non-zero value) or { layer: { value|name: "tag" | { tag, oneWay?, sensor?, friction?, restitution? } } }.
function rulesOf(L, rule, what) {
  if (typeof rule === "string") return [{ match: (v) => v !== 0, tag: rule }];
  if (!rule || typeof rule !== "object") fail(`${what}: collide.${L.def.id} must be a tag or { value: tag | { tag, oneWay?, sensor?, friction?, restitution? } }, got ${JSON.stringify(rule)}`);
  return Object.entries(rule).map(([key, r]) => {
    const v = intValue(L, /^\d+$/.test(key) ? Number(key) : key, `${what}: collide.${L.def.id}`);
    const body = typeof r === "string" ? { tag: r } : r;
    if (typeof body?.tag !== "string") fail(`${what}: collide.${L.def.id}.${key} needs a tag, got ${JSON.stringify(r)}`);
    return { match: (x) => x === v, ...body };
  });
}

function buildColliders(rec, L) {
  for (const id of L.colliders) N.destroy(id);
  L.colliders = [];
  const g = L.def.grid;
  for (const r of L.rules) {
    const merged = N.mergeCells(L.def.cells[0], L.ints.map((v) => (r.match(v) ? 1 : 0)));
    for (let i = 0; i < merged.length; i += 4) {
      const [w, h] = [merged[i + 2] * g, merged[i + 3] * g];
      const pos = [rec.pos[0] + L.def.offset[0] + merged[i] * g + w / 2, rec.pos[1] + L.def.offset[1] + merged[i + 1] * g + h / 2];
      const collider = { rect: [w, h], oneWay: r.oneWay, sensor: r.sensor, friction: r.friction, restitution: r.restitution };
      L.colliders.push(N.spawn({ tags: [r.tag], pos, body: { type: "static", collider } }));
    }
  }
}

// ---- views ---------------------------------------------------------------

function liveLayers(level, copy) {
  return new Map(level.layers.map((def) => [def.id, {
    def, ints: copy && def.ints ? [...def.ints] : def.ints, tiles: copy ? [...def.tiles] : def.tiles, colliders: [], rules: [],
  }]));
}

function cellOf(view, pos, layer, what) {
  const L = layer === undefined ? view.layers.values().next().value : layerOf(view, layer, null, what);
  if (!L) fail(`${what}: level "${view.level.id}" has no layers`);
  return { L, cell: [Math.floor((pos[0] - view.pos[0] - L.def.offset[0]) / L.def.grid), Math.floor((pos[1] - view.pos[1] - L.def.offset[1]) / L.def.grid)] };
}

function topTile(view, L, cx, cy) {
  const g = L.def.grid;
  for (let i = L.tiles.length - 5; i >= 0; i -= 5) {
    if (Math.floor(L.tiles[i] / g) !== cx || Math.floor(L.tiles[i + 1] / g) !== cy) continue;
    const ts = L.def.tileset;
    const id = L.tiles[i + 2];
    return { id, flipX: (L.tiles[i + 3] & 1) !== 0, flipY: (L.tiles[i + 3] & 2) !== 0, tags: [...(ts.tags.get(id) ?? [])], data: ts.data.get(id) ?? null };
  }
  return null;
}

// Read-only level view (ctx.world(...).level(id) and the base of a spawned level).
function readView(view, world) {
  const what = (fn) => `level("${view.level.id}").${fn}`;
  const l = view.level;
  return {
    id: l.id, iid: l.iid, depth: l.depth, size: [...l.size], bg: l.bg, fields: clone(l.fields),
    get pos() { return [...view.pos]; },
    layers: [...view.layers.keys()],
    neighbours: l.neighbours.map((n) => ({ level: world.levels.find((x) => x.iid === n.iid)?.id ?? null, dir: n.dir })),
    entities(id) {
      if (id !== undefined && !world.entityIds.has(id)) fail(`${what("entities")}("${id}"): no entity "${id}" in the project; it has: ${list([...world.entityIds])}`);
      const out = [];
      for (const L of view.layers.values()) for (const e of L.def.entities) if (id === undefined || e.id === id) out.push(worldEntity(view, L, e));
      return out;
    },
    int(layer, cx, cy) {
      const L = layerOf(view, layer, "ints", what("int"));
      const [w, h] = L.def.cells;
      return cx < 0 || cy < 0 || cx >= w || cy >= h ? 0 : L.ints[Math.floor(cy) * w + Math.floor(cx)];
    },
    tile(layer, cx, cy) {
      const L = layerOf(view, layer, "tiles", what("tile"));
      cellIndex(L, cx, cy, what("tile"));
      return topTile(view, L, cx, cy);
    },
    toCell: (pos, layer) => cellOf(view, pos, layer, what("toCell")).cell,
    toWorld(cell, layer) {
      const { L } = cellOf(view, view.pos, layer, what("toWorld"));
      const g = L.def.grid;
      return [view.pos[0] + L.def.offset[0] + (cell[0] + 0.5) * g, view.pos[1] + L.def.offset[1] + (cell[1] + 0.5) * g];
    },
  };
}

export function levelView(world, level) {
  return readView({ level, pos: level.pos, layers: liveLayers(level, false) }, world);
}

export function worldView(world) {
  const view = (l) => levelView(world, l);
  return {
    name: world.name, layout: world.layout, levels: world.levels.map((l) => l.id),
    level: (id) => view(levelOf(world, id, `world("${world.name}").level`)),
    levelAt(pos) {
      const l = world.levels.find((x) => pos[0] >= x.pos[0] && pos[1] >= x.pos[1] && pos[0] < x.pos[0] + x.size[0] && pos[1] < x.pos[1] + x.size[1]);
      return l ? view(l) : null;
    },
    entity(iid) {
      for (const l of world.levels) {
        const found = view(l).entities().find((e) => e.iid === iid);
        if (found) return { ...found, level: l.id };
      }
      return null;
    },
  };
}

// ---- spawned levels --------------------------------------------------------

function setTile(rec, L, cx, cy, tile, opts, what) {
  cellIndex(L, cx, cy, what);
  const g = L.def.grid;
  const kept = [];
  for (let i = 0; i < L.tiles.length; i += 5) {
    if (Math.floor(L.tiles[i] / g) !== cx || Math.floor(L.tiles[i + 1] / g) !== cy) kept.push(...L.tiles.slice(i, i + 5));
  }
  if (tile !== null) {
    if (!Number.isInteger(tile) || tile < 0) fail(`${what}: tile must be a tile id (whole number >= 0) or null, got ${JSON.stringify(tile)}`);
    kept.push(cx * g, cy * g, tile, (opts?.flipX ? 1 : 0) | (opts?.flipY ? 2 : 0), opts?.alpha ?? 1);
  }
  L.tiles = kept;
  redraw(rec, L);
}

function spawnEntities(rec, view, factories, spawn, what) {
  for (const key of Object.keys(factories ?? {})) {
    if (!rec.world.entityIds.has(key)) fail(`${what}: entities.${key}: no entity "${key}" in the project; it has: ${list([...rec.world.entityIds])}`);
  }
  for (const e of view.entities()) {
    const make = factories?.[e.id];
    if (!make) continue;
    const specs = make(e);
    for (const spec of [specs].flat()) if (spec) rec.spawned.set(e.iid, spawn(spec));
  }
}

function checkCollide(rec, opts, what) {
  for (const [layer, rule] of Object.entries(opts.collide ?? {})) {
    const L = layerOf(rec, layer, "ints", `${what}: collide`);
    L.rules = rulesOf(L, rule, what);
    buildColliders(rec, L);
  }
}

// opts: { pos?, z?, layer?, layers?, collide?, entities?, background? }; helpers: { spawn, kill, time }.
export function spawnLevel(world, level, opts, helpers) {
  const what = `spawnLevel("${world.name}", "${level.id}")`;
  const rec = {
    world, level, pos: opts.pos ? [...opts.pos] : [...level.pos], z: opts.z ?? 0, layers: liveLayers(level, true),
    target: (id) => opts.layers?.[id] ?? opts.layer, start: helpers.time, time: 0, spawned: new Map(), own: [],
  };
  if (opts.background === true && level.bg) {
    rec.own.push(N.spawn({ tags: [], pos: [rec.pos[0] + level.size[0] / 2, rec.pos[1] + level.size[1] / 2], z: rec.z, layer: opts.layer,
      shape: { rect: [...level.size], color: level.bg } }));
  }
  for (const L of rec.layers.values()) drawLayer(rec, L);
  checkCollide(rec, opts, what);
  const base = readView(rec, world);
  spawnEntities(rec, base, opts.entities, helpers.spawn, what);
  return { rec, view: liveView(rec, base, helpers) };
}

function liveView(rec, base, helpers) {
  const what = (fn) => `level("${rec.level.id}").${fn}`;
  return Object.assign(base, {
    setInt(layer, cx, cy, value) {
      const L = layerOf(rec, layer, "ints", what("setInt"));
      L.ints[cellIndex(L, cx, cy, what("setInt"))] = intValue(L, value, what("setInt"));
      if (L.rules.length) buildColliders(rec, L);
    },
    setTile(layer, cx, cy, tile, opts) {
      setTile(rec, layerOf(rec, layer, "tiles", what("setTile")), cx, cy, tile, opts, what("setTile"));
    },
    entity: (iid) => rec.spawned.get(iid) ?? null,
    destroy() {
      for (const L of rec.layers.values()) {
        for (const id of [L.drawId, L.animId, ...L.colliders]) if (id !== undefined) N.destroy(id);
        L.drawId = L.animId = undefined;
        L.colliders = [];
      }
      for (const id of rec.own) N.destroy(id);
      rec.own = [];
      for (const e of rec.spawned.values()) helpers.kill(e);
      rec.spawned.clear();
      rec.dead = true;
    },
  });
}
