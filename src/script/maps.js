// Module "alpha:maps": Tiled maps (.tmj JSON, tilesets embedded or external
// .tsj) for the runtime. Parsed once at boot into plain data; spawnMap turns
// tile layers into one batched entity each and collision layers into merged
// static boxes. Keep this file ASCII (it is embedded as C++ literals).
const N = globalThis.__native;

const GID_MASK = 0x1fffffff;  // the top 3 bits of a gid are Tiled's flip flags

function fail(message) {
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
    fail(`${what}: ${file} is not valid JSON (${e.message}); export the map from Tiled as JSON (.tmj / .tsj)`);
  }
}

function tileset(ref, mapFile, what) {
  let ts = ref;
  let base = dirOf(mapFile);
  if (ref.source) {
    const file = joinPath(base, ref.source);
    if (!file.endsWith(".tsj") && !file.endsWith(".json")) fail(`${what}: tileset ${ref.source} must be saved as JSON (.tsj), not .tsx`);
    ts = readJson(file, what);
    base = dirOf(file);
  }
  if (typeof ts.image !== "string") fail(`${what}: tileset "${ts.name}" has no single image (image-collection tilesets are not supported)`);
  return {
    firstgid: ref.firstgid, image: joinPath(base, ts.image), tile: [ts.tilewidth, ts.tileheight],
    columns: ts.columns, margin: ts.margin ?? 0, spacing: ts.spacing ?? 0,
  };
}

function properties(list) {
  const out = {};
  for (const p of list ?? []) out[p.name] = p.value;
  return out;
}

// Tiled object -> plain data scripts read (x, y = top-left in map pixels; point objects: the point).
function object(o) {
  return {
    id: o.id, name: o.name ?? "", type: o.type ?? o.class ?? "", x: o.x, y: o.y,
    width: o.width ?? 0, height: o.height ?? 0, rotation: o.rotation ?? 0, point: Boolean(o.point),
    properties: properties(o.properties),
  };
}

// Layers in draw order; groups are flattened (their offsets added).
function layers(list, what, dx = 0, dy = 0, out = []) {
  for (const l of list ?? []) {
    const offset = [dx + (l.offsetx ?? 0), dy + (l.offsety ?? 0)];
    if (l.type === "group") layers(l.layers, what, offset[0], offset[1], out);
    else if (l.type === "tilelayer") out.push(tileLayer(l, what, offset));
    else if (l.type === "objectgroup") out.push({ kind: "objects", name: l.name, objects: (l.objects ?? []).map(object) });
  }
  return out;
}

function tileLayer(l, what, offset) {
  if (l.encoding && l.encoding !== "csv") {
    fail(`${what}: layer "${l.name}" uses ${l.encoding}${l.compression ? "/" + l.compression : ""} data; set "Tile Layer Format" to CSV in Tiled's map properties`);
  }
  return { kind: "tiles", name: l.name, visible: l.visible !== false, data: l.data, width: l.width, offset };
}

export function loadMap(name, file) {
  const what = `assets.maps.${name} (${file})`;
  const m = readJson(file, what);
  if (m.orientation !== "orthogonal") fail(`${what}: only orthogonal maps are supported, got "${m.orientation}"`);
  if (m.infinite) fail(`${what}: infinite maps are not supported; uncheck "Infinite" in Tiled's map properties`);
  const tilesets = (m.tilesets ?? []).map((t) => tileset(t, file, what)).sort((a, b) => a.firstgid - b.firstgid);
  return {
    name, width: m.width, height: m.height, tile: [m.tilewidth, m.tileheight],
    size: [m.width * m.tilewidth, m.height * m.tileheight], tilesets, layers: layers(m.layers, what),
    properties: properties(m.properties),
  };
}

function names(map, kind) {
  return map.layers.filter((l) => l.kind === kind).map((l) => l.name).join(", ") || "(none)";
}

function layerOf(map, name, kind, what) {
  const l = map.layers.find((x) => x.name === name && x.kind === kind);
  if (!l) fail(`${what}: map "${map.name}" has no ${kind === "tiles" ? "tile" : "object"} layer "${name}"; it has: ${names(map, kind)}`);
  return l;
}

function tilesetOf(map, gid) {
  let found = null;
  for (const ts of map.tilesets) if (ts.firstgid <= gid) found = ts;
  return found;
}

// Cells of one layer grouped by tileset: Map(tileset -> [dx, dy, sx, sy, ...]).
function cellsByTileset(map, layer) {
  const groups = new Map();
  layer.data.forEach((raw, i) => {
    const gid = raw & GID_MASK;
    const ts = gid ? tilesetOf(map, gid) : null;
    if (!ts) return;
    const local = gid - ts.firstgid;
    const cells = groups.get(ts) ?? [];
    cells.push((i % layer.width) * map.tile[0], Math.floor(i / layer.width) * map.tile[1] + map.tile[1] - ts.tile[1],
      ts.margin + (local % ts.columns) * (ts.tile[0] + ts.spacing),
      ts.margin + Math.floor(local / ts.columns) * (ts.tile[1] + ts.spacing));
    groups.set(ts, cells);
  });
  return groups;
}

function drawLayer(map, layer, pos, z, target) {
  for (const [ts, cells] of cellsByTileset(map, layer)) {
    N.tileLayer({ image: ts.image, tile: ts.tile, size: map.size, pos: [pos[0] + layer.offset[0], pos[1] + layer.offset[1]], z, cells, layer: target, pixelMode: map.pixelMode });
  }
}

// Static boxes over the layer's non-empty cells, merged into few rects, tagged with the layer name.
function collideLayer(map, layer, pos, spawn) {
  const solid = layer.data.map((g) => ((g & GID_MASK) ? 1 : 0));
  const r = N.mergeCells(layer.width, solid);
  const [tw, th] = map.tile;
  for (let i = 0; i < r.length; i += 4) {
    const [w, h] = [r[i + 2] * tw, r[i + 3] * th];
    spawn({ tags: [layer.name], pos: [pos[0] + layer.offset[0] + r[i] * tw + w / 2, pos[1] + layer.offset[1] + r[i + 1] * th + h / 2],
      body: { type: "static", collider: { rect: [w, h] } } });
  }
}

export function spawnLayers(map, opts, spawn) {
  const what = `spawnMap("${map.name}")`;
  const pos = opts.pos ?? [0, 0];
  for (const name of opts.collide ?? []) collideLayer(map, layerOf(map, name, "tiles", what), pos, spawn);
  // opts.layer: the config.layers layer of every tile layer; opts.layers: per Tiled layer name.
  for (const l of map.layers) if (l.kind === "tiles" && l.visible) drawLayer(map, l, pos, opts.z ?? 0, opts.layers?.[l.name] ?? opts.layer);
}

// What ctx.map(name) returns: sizes and object layers as plain data.
export function mapView(map) {
  return {
    name: map.name, width: map.width, height: map.height, tileSize: [...map.tile], size: [...map.size],
    properties: { ...map.properties },
    layers: map.layers.map((l) => l.name),
    objects: (layer) => layerOf(map, layer, "objects", `map("${map.name}").objects("${layer}")`).objects.map((o) => ({ ...o, properties: { ...o.properties } })),
  };
}
