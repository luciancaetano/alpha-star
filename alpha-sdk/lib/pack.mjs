// Exporter: compiles a game and assembles a self-contained pack directory
//   <out>/main.js        the bundled game
//   <out>/<asset paths>  every file declared in game.assets, same relative paths
// The engine runs a pack with: alpha_star <out>
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import * as esbuild from "esbuild";
import { build } from "./build.mjs";
import { availableTargets, bundledEngine, engineFile } from "./engine.mjs";

const SHIM = path.join(path.dirname(fileURLToPath(import.meta.url)), "alpha-shim.mjs");

const problem = (message, file = "") => ({ file, line: 0, column: 0, message });

// Bundles the game with "alpha" replaced by the shim and imports it in Node.
export async function readGameDef(entry) {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-def-"));
  const outfile = path.join(tmp, "def.mjs");
  const shim = {
    name: "alpha-shim",
    setup(b) {
      b.onResolve({ filter: /^alpha(\/.*)?$/ }, () => ({ path: SHIM }));
    },
  };
  try {
    await esbuild.build({ entryPoints: [entry], outfile, bundle: true, format: "esm", platform: "node",
                          plugins: [shim], logLevel: "silent" });
    const mod = await import(pathToFileURL(outfile).href);
    return mod.default?.def;
  } finally {
    fs.rmSync(tmp, { recursive: true, force: true });
  }
}

// Files the pack must contain: every string sound, image, font and UI document
// (the files a document references are found by uiReferences once it exists).
export function assetFiles(def) {
  const assets = def?.assets ?? {};
  const files = [];
  for (const [name, sound] of Object.entries(assets.sounds ?? {})) {
    if (typeof sound === "string") files.push({ key: `assets.sounds.${name}`, file: sound });
  }
  for (const [name, image] of Object.entries(assets.images ?? {})) {
    files.push({ key: `assets.images.${name}`, file: image });
  }
  for (const kind of ["fonts", "ui"]) {
    for (const [name, file] of Object.entries(assets[kind] ?? {})) files.push({ key: `assets.${kind}.${name}`, file });
  }
  return files;
}

// Relative file references in RML (`href=`, `src=`) and RCSS (`url(...)`, `image(...)`).
const REFERENCE = /\b(?:href|src)\s*=\s*"([^"]*)"|\b(?:url|image)\(\s*["']?([^"')\s]+)["']?\s*\)/g;

function referencesIn(text) {
  return [...text.matchAll(REFERENCE)].map((m) => m[1] ?? m[2])
    .filter((ref) => ref && !ref.startsWith("#") && !/^[a-z]+:/i.test(ref));
}

// Every file a UI document pulls in, transitively (an .rcss may reference
// images), as { key, file } with paths relative to the project; or a problem.
function uiReferences(root, { key, file }) {
  const found = [];
  const queue = [path.normalize(file)];
  const seen = new Set(queue);
  while (queue.length) {
    const doc = queue.shift();
    if (!/\.(rml|rcss)$/i.test(doc)) continue;
    for (const ref of referencesIn(fs.readFileSync(path.join(root, doc), "utf8"))) {
      const target = path.normalize(path.join(path.dirname(doc), ref));
      if (seen.has(target)) continue;
      seen.add(target);
      const shown = doc.split(path.sep).join("/");
      const missing = target.startsWith("..") || !fs.existsSync(path.join(root, target));
      if (missing) return { problem: problem(`${key}: ${shown} references "${ref}", not found inside the project`) };
      found.push({ key, file: target });
      queue.push(target);
    }
  }
  return { found };
}

// Declared files plus what the UI documents reference; problems if any is missing.
function packFiles(root, def) {
  const files = assetFiles(def);
  const problems = files.map((f) => checkAsset(root, f)).filter(Boolean);
  if (problems.length) return { files, problems };
  for (const f of files.filter((x) => x.key.startsWith("assets.ui."))) {
    const { found, problem: p } = uiReferences(root, f);
    if (p) return { files, problems: [p] };
    files.push(...found);
  }
  return { files, problems: [] };
}

// A pack must be self-contained: relative paths that stay inside the project.
function checkAsset(root, { key, file }) {
  const norm = path.normalize(file);
  if (path.isAbsolute(file) || norm.startsWith("..")) {
    return problem(`${key}: "${file}" must be a relative path inside the project`);
  }
  if (!fs.existsSync(path.join(root, norm))) {
    return problem(`${key}: file "${file}" not found (paths are relative to ${root})`);
  }
  return null;
}

function copyAssets(root, out, files) {
  for (const { file } of files) {
    const dest = path.join(out, path.normalize(file));
    fs.mkdirSync(path.dirname(dest), { recursive: true });
    fs.copyFileSync(path.join(root, path.normalize(file)), dest);
  }
}

// Standalone pack: the engine binary sits next to main.js and, started with no
// arguments, runs it (like a fused LOVE game).
function engineFor(target) {
  const engine = bundledEngine(target);
  if (engine) return { engine };
  const have = availableTargets().join(", ") || "(none; stage one with tools/stage_engine.py)";
  return { problem: problem(`no bundled engine for target "${target}"; SDK has: ${have}`) };
}

// root: project directory (asset paths are relative to it). target: null for
// a plain pack, or "<platform>-<arch>" to include that engine binary.
// Returns { ok, problems, files }. Writes nothing unless everything checks out.
export async function exportPack({ root, entry, out, target = null }) {
  const standalone = target ? engineFor(target) : {};
  if (standalone.problem) return { ok: false, problems: [standalone.problem], files: [] };
  const staging = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-pack-"));
  try {
    const built = await build(entry, path.join(out, "main.js"), path.join(staging, "main.js"));
    if (!built.ok) return { ok: false, problems: built.problems, files: [] };
    const def = await readGameDef(entry);
    if (!def) return { ok: false, problems: [problem("main module must `export default game({...})`", entry)], files: [] };
    const { files, problems } = packFiles(root, def);
    if (problems.length) return { ok: false, problems, files };
    fs.rmSync(out, { recursive: true, force: true });
    fs.mkdirSync(out, { recursive: true });
    fs.copyFileSync(path.join(staging, "main.js"), path.join(out, "main.js"));
    copyAssets(root, out, files);
    if (standalone.engine) {
      const dest = path.join(out, engineFile(target));
      fs.copyFileSync(standalone.engine, dest);
      fs.chmodSync(dest, 0o755);
    }
    return { ok: true, problems: [], files };
  } finally {
    fs.rmSync(staging, { recursive: true, force: true });
  }
}
