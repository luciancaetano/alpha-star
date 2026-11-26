// Files an LDtk project (.ldtk) pulls into a pack: every tileset image and
// every external level (.ldtkl), with paths relative to the project file.
import fs from "node:fs";
import path from "node:path";

const problem = (message) => ({ file: "", line: 0, column: 0, message });

function readJson(root, file, key) {
  try {
    return { json: JSON.parse(fs.readFileSync(path.join(root, file), "utf8")) };
  } catch (e) {
    return { problem: problem(`${key}: ${file.split(path.sep).join("/")} is not readable JSON (${e.message}); save the project from LDtk`) };
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

/** Every file a world references, as { key, file }; or { problem }. */
export function worldReferences(root, { key, file }) {
  const worldFile = path.normalize(file);
  const read = readJson(root, worldFile, key);
  if (read.problem) return read;
  const refs = [
    ...(read.json.defs?.tilesets ?? []).map((t) => t.relPath).filter((r) => typeof r === "string"),
    ...(read.json.levels ?? []).map((l) => l.externalRelPath).filter((r) => typeof r === "string"),
  ];
  const found = [];
  for (const ref of refs) {
    const r = resolve(root, worldFile, ref, key);
    if (r.problem) return { problem: r.problem };
    found.push({ key, file: r.target });
  }
  return { found };
}
