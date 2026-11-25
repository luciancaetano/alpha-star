// Files a Tiled map (.tmj) pulls into a pack: external tilesets (.tsj) and
// every tileset image, with paths relative to the file that names them.
import fs from "node:fs";
import path from "node:path";

const problem = (message) => ({ file: "", line: 0, column: 0, message });

function readJson(root, file, key) {
  try {
    return { json: JSON.parse(fs.readFileSync(path.join(root, file), "utf8")) };
  } catch (e) {
    return { problem: problem(`${key}: ${file.split(path.sep).join("/")} is not readable JSON (${e.message}); export from Tiled as JSON`) };
  }
}

// `ref` named in `from` (project-relative), resolved; or a problem.
function resolve(root, from, ref, key) {
  const target = path.normalize(path.join(path.dirname(from), ref));
  const shown = from.split(path.sep).join("/");
  if (target.startsWith("..") || !fs.existsSync(path.join(root, target))) {
    return { problem: problem(`${key}: ${shown} references "${ref}", not found inside the project`) };
  }
  return { target };
}

function tilesetFiles(root, mapFile, ts, key) {
  const found = [];
  let owner = mapFile;
  let data = ts;
  if (ts.source) {
    if (/\.tsx$/i.test(ts.source)) return { problem: problem(`${key}: ${ts.source}: save tilesets as JSON (.tsj) in Tiled`) };
    const r = resolve(root, mapFile, ts.source, key);
    if (r.problem) return r;
    const read = readJson(root, r.target, key);
    if (read.problem) return read;
    found.push(r.target);
    owner = r.target;
    data = read.json;
  }
  if (typeof data.image === "string") {
    const r = resolve(root, owner, data.image, key);
    if (r.problem) return r;
    found.push(r.target);
  }
  return { found };
}

/** Every file a map references, as { key, file }; or { problem }. */
export function mapReferences(root, { key, file }) {
  const mapFile = path.normalize(file);
  const read = readJson(root, mapFile, key);
  if (read.problem) return read;
  const found = [];
  for (const ts of read.json.tilesets ?? []) {
    const r = tilesetFiles(root, mapFile, ts, key);
    if (r.problem) return { problem: r.problem };
    found.push(...r.found.map((f) => ({ key, file: f })));
  }
  return { found };
}
