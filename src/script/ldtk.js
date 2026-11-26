// Module "alpha:ldtk": LDtk projects (.ldtk JSON, levels embedded or in
// external .ldtkl files) parsed once at boot into plain data the runtime
// reads. Tiles keep LDtk's precomputed positions (`px`, `src`, flips, alpha),
// so drawing needs no gid math. Keep this file ASCII (embedded as C++ literals).
const N = globalThis.__native;

const LAYOUTS = ["Free", "GridVania", "LinearHorizontal", "LinearVertical"];

export function fail(message) {
  throw new Error(message);
}

function dirOf(file) {
  const i = file.lastIndexOf("/");
  return i < 0 ? "" : file.slice(0, i + 1);
}

// dir + relative path, with "." and ".." resolved ("maps/../img/a.png" -> "img/a.png").
export function joinPath(dir, rel) {
  const out = [];
  for (const part of (dir + rel).replace(/\\/g, "/").split("/")) {
    if (part === "..") out.pop();
    else if (part !== "." && part !== "") out.push(part);
  }
  return out.join("/");
}

function readJson(file, what) {
  const text = N.readText(file);
  try {
    return JSON.parse(text);
  } catch (e) {
    fail(`${what}: ${file} is not valid JSON (${e.message}); save the project from LDtk (.ldtk / .ldtkl)`);
  }
}

// Tile custom data that animates it: {"frames": [tileIds...], "fps": n}.
function animOf(text, what, tileId) {
  if (typeof text !== "string" || !text.trim().startsWith("{")) return null;
  let d;
  try {
    d = JSON.parse(text);
  } catch {
    return null;
  }
  if (d.frames === undefined) return null;
  if (!Array.isArray(d.frames) || !d.frames.length || !d.frames.every(Number.isInteger) || !(d.fps > 0)) {
    fail(`${what}: tile ${tileId} custom data must be {"frames": [tile ids], "fps": number > 0}, got ${text}`);
  }
  return { frames: d.frames, fps: d.fps };
}

function tileset(t, dir, what) {
  const ts = {
    uid: t.uid, id: t.identifier, image: t.relPath ? joinPath(dir, t.relPath) : null, grid: t.tileGridSize,
    spacing: t.spacing ?? 0, padding: t.padding ?? 0, columns: t.__cWid, tags: new Map(), data: new Map(), anims: new Map(),
  };
  for (const tag of t.enumTags ?? []) {
    for (const id of tag.tileIds) ts.tags.set(id, [...(ts.tags.get(id) ?? []), tag.enumValueId]);
  }
  for (const c of t.customData ?? []) {
    ts.data.set(c.tileId, c.data);
    const anim = animOf(c.data, `${what}: tileset "${t.identifier}"`, c.tileId);
    if (anim) ts.anims.set(c.tileId, anim);
  }
  return ts;
}

// Top-left of tile `id` in the tileset image.
export function tileSrc(ts, id) {
  const step = ts.grid + ts.spacing;
  return [ts.padding + (id % ts.columns) * step, ts.padding + Math.floor(id / ts.columns) * step];
}

// LDtk field value -> plain script data (points as cells, refs as iids, paths resolved).
function fieldValue(type, v, dir, tilesets) {
  if (v === null || v === undefined) return null;
  const inner = /^Array<(.*)>$/.exec(type);
  if (inner) return v.map((x) => fieldValue(inner[1], x, dir, tilesets));
  if (type === "Point") return [v.cx, v.cy];
  if (type === "EntityRef") return { entity: v.entityIid, layer: v.layerIid, level: v.levelIid };
  if (type === "FilePath") return joinPath(dir, v);
  if (type === "Tile") return { tileset: tilesets.get(v.tilesetUid)?.id ?? null, rect: [v.x, v.y, v.w, v.h] };
  return v;
}

function fields(list, dir, tilesets) {
  const out = {};
  for (const f of list ?? []) out[f.__identifier] = fieldValue(f.__type, f.__value, dir, tilesets);
  return out;
}

function entity(e, dir, tilesets) {
  return {
    id: e.__identifier, iid: e.iid, local: [...e.px], grid: [...e.__grid], size: [e.width, e.height],
    pivot: [...e.__pivot], tags: [...(e.__tags ?? [])], fields: fields(e.fieldInstances, dir, tilesets),
    tile: e.__tile ? { tileset: tilesets.get(e.__tile.tilesetUid)?.id ?? null, rect: [e.__tile.x, e.__tile.y, e.__tile.w, e.__tile.h] } : null,
  };
}

// Flat [px.x, px.y, tileId, flips, alpha, ...] in draw order.
function tilesOf(list) {
  const out = [];
  for (const t of list ?? []) out.push(t.px[0], t.px[1], t.t, t.f ?? 0, t.a ?? 1);
  return out;
}

function layer(l, w) {
  const def = w.layerDefs.get(l.layerDefUid);
  const ts = w.tilesets.get(l.overrideTilesetUid ?? l.__tilesetDefUid) ?? null;
  const tiles = tilesOf(l.__type === "Tiles" ? l.gridTiles : l.autoLayerTiles);
  if (tiles.length && (!ts || !ts.image)) fail(`${w.what}: layer "${l.__identifier}" draws tiles from a tileset with no image file (LDtk's internal icons are not supported)`);
  return {
    id: l.__identifier, iid: l.iid, type: l.__type, grid: l.__gridSize, cells: [l.__cWid, l.__cHei],
    offset: [l.__pxTotalOffsetX ?? 0, l.__pxTotalOffsetY ?? 0], opacity: l.__opacity ?? 1, visible: l.visible !== false,
    tileset: ts, tiles, ints: l.__type === "IntGrid" ? [...l.intGridCsv] : null,
    values: def?.values ?? new Map(), entities: (l.entityInstances ?? []).map((e) => entity(e, w.dir, w.tilesets)),
  };
}

function level(raw, w) {
  let l = raw;
  if (l.externalRelPath) l = { ...readJson(joinPath(w.dir, l.externalRelPath), w.what), worldX: raw.worldX, worldY: raw.worldY };
  if (!l.layerInstances) fail(`${w.what}: level "${l.identifier}" has no layers (save the project again in LDtk)`);
  return {
    id: l.identifier, iid: l.iid, pos: [l.worldX, l.worldY], depth: l.worldDepth ?? 0, size: [l.pxWid, l.pxHei],
    bg: l.__bgColor ?? null, fields: fields(l.fieldInstances, w.dir, w.tilesets),
    neighbours: (l.__neighbours ?? []).map((n) => ({ iid: n.levelIid, dir: n.dir })),
    layers: l.layerInstances.map((x) => layer(x, w)).reverse(),  // LDtk lists the top layer first; ours draw in order
  };
}

// Linear layouts store no usable world position: lay levels end to end.
function placeLinear(levels, layout) {
  let at = 0;
  for (const l of levels) {
    l.pos = layout === "LinearHorizontal" ? [at, 0] : [0, at];
    at += layout === "LinearHorizontal" ? l.size[0] : l.size[1];
  }
}

function checkVersion(p, what) {
  const major = Number(String(p.jsonVersion ?? "0").split(".")[0]);
  if (!(major >= 1)) fail(`${what}: LDtk JSON version ${p.jsonVersion ?? "(none)"} is too old; open and save it in LDtk 1.0 or newer`);
  if (!p.levels?.length && p.worlds?.length) fail(`${what}: multi-world projects are not supported; turn off "Multi-worlds" in LDtk's project settings`);
}

export function loadWorld(name, file) {
  const what = `assets.worlds.${name} (${file})`;
  const p = readJson(file, what);
  checkVersion(p, what);
  const w = { what, dir: dirOf(file), tilesets: new Map(), layerDefs: new Map() };
  for (const t of p.defs?.tilesets ?? []) w.tilesets.set(t.uid, tileset(t, w.dir, what));
  for (const d of p.defs?.layers ?? []) {
    w.layerDefs.set(d.uid, { values: new Map((d.intGridValues ?? []).map((v) => [v.value, v.identifier ?? null])) });
  }
  const layout = LAYOUTS.includes(p.worldLayout) ? p.worldLayout : "Free";
  const levels = (p.levels ?? []).map((l) => level(l, w));
  if (layout.startsWith("Linear")) placeLinear(levels, layout);
  const entityIds = new Set((p.defs?.entities ?? []).map((e) => e.identifier));
  return { name, layout, levels, tilesets: w.tilesets, entityIds };
}
