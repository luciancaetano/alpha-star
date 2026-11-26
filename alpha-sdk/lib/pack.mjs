// Exporter: compiles a game and assembles a self-contained pack directory
//   <out>/main.js        the bundled game
//   <out>/<asset paths>  every file declared in game.assets, plus the files UI
//                        documents reference (<link href>, src=, url(), image()),
//                        BMFont pages and font license files, same relative paths
//   <out>/<shader>.spv compiled assets.shaders (lib/shaders.mjs; no .frag)
//   <out>/THIRD_PARTY_NOTICES.txt  engine dependency notices + font licenses
// The engine runs a pack with: alpha_star <out>
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import * as esbuild from "esbuild";
import { buildAtlas } from "./atlas.mjs";
import { build } from "./build.mjs";
import { mapReferences } from "./maps.mjs";
import { BYTECODE_FILE, compileBytecode } from "./bytecode.mjs";
import { bundledEngine, describeTargets, engineFile, shippingEngine } from "./engine.mjs";
import { compileShaders, glslangTool, shadercrossTool } from "./shaders.mjs";

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

// "file" or { file, filter? / pixelMode? } (images, maps, fonts): the file and its filtering.
const entryFile = (entry) => (typeof entry === "string" ? entry : entry?.file ?? "");
const entryPixel = (entry, fallback) => (typeof entry?.pixelMode === "boolean" ? entry.pixelMode : fallback);
const FILTERS = ["nearest", "linear", "sharp"];
const entryFilter = (entry, fallback) => (FILTERS.includes(entry?.filter) ? entry.filter : fallback);

// Files the pack must contain: every sound file, image, font, music, locale and UI document
// (the files a document references are found by uiReferences once it exists).
// Images and sheets carry `filter` (their own, or config.pixelMode: true = nearest, false = linear).
export function assetFiles(def) {
  const assets = def?.assets ?? {};
  const pixel = entryPixel(def?.config, true) ? "nearest" : "linear";
  const files = [];
  for (const [name, sound] of Object.entries(assets.sounds ?? {})) {
    const file = typeof sound === "string" ? sound : sound?.file;
    if (typeof file === "string") files.push({ key: `assets.sounds.${name}`, file });
  }
  for (const [name, image] of Object.entries(assets.images ?? {})) {
    files.push({ key: `assets.images.${name}`, file: entryFile(image), filter: entryFilter(image, pixel) });
  }
  for (const [name, sheet] of Object.entries(assets.sheets ?? {})) {
    files.push({ key: `assets.sheets.${name}.image`, file: sheet?.image ?? "", filter: entryFilter(sheet, pixel) });
  }
  for (const kind of ["fonts", "ui", "maps", "music", "locales"]) {
    for (const [name, entry] of Object.entries(assets[kind] ?? {})) files.push({ key: `assets.${kind}.${name}`, file: entryFile(entry) });
  }
  for (const [name, font] of Object.entries(assets.fonts ?? {})) {
    if (typeof font?.license === "string") files.push({ key: `assets.fonts.${name}.license`, file: font.license });
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
  for (const f of files.filter((x) => /^assets\.maps\.[^.]+$/.test(x.key))) {
    const { found, problem: p } = mapReferences(root, f);
    if (p) return { files, problems: [p] };
    files.push(...found);
  }
  for (const f of files.filter((x) => /^assets\.fonts\.[^.]+$/.test(x.key) && /\.fnt$/i.test(x.file))) {
    const { found, problem: p } = fontPages(root, f);
    if (p) return { files, problems: [p] };
    files.push(...found);
  }
  return { files, problems: [] };
}

// The PNG pages a BMFont .fnt (text or XML) names in `page ... file="..."`.
function fontPages(root, { key, file }) {
  const text = fs.readFileSync(path.join(root, path.normalize(file)), "utf8");
  const found = [];
  for (const m of text.matchAll(/\bpage\b[^\n>]*\bfile\s*=\s*"([^"]+)"/g)) {
    const target = path.normalize(path.join(path.dirname(file), m[1]));
    if (target.startsWith("..") || !fs.existsSync(path.join(root, target))) {
      return { problem: problem(`${key}: ${posix(file)} references "${m[1]}", not found inside the project`) };
    }
    found.push({ key, file: target });
  }
  return { found };
}

const NOTICES = path.join(path.dirname(fileURLToPath(import.meta.url)), "..", "notices");

// THIRD_PARTY_NOTICES.txt: the notices of everything the engine links (alpha-sdk/notices,
// made by tools/make_notices.py from the dependencies' own license files), then each
// font's license file when assets.fonts.<name>.license names one.
function writeNotices(root, out, def) {
  const parts = ["This game runs on the alpha-star engine, which includes the software below.\n" +
                 "Each part keeps its own license; the full texts follow."];
  for (const name of fs.readdirSync(NOTICES).filter((n) => n.endsWith(".txt")).sort()) {
    parts.push(fs.readFileSync(path.join(NOTICES, name), "utf8").trim());
  }
  for (const [name, font] of Object.entries(def?.assets?.fonts ?? {})) {
    if (typeof font?.license !== "string") continue;
    const title = `Font "${name}" (${posix(font.file)})`;
    const text = fs.readFileSync(path.join(root, path.normalize(font.license)), "utf8").trim();
    parts.push(`${title}\n${"=".repeat(title.length)}\n\n${text}`);
  }
  fs.writeFileSync(path.join(out, "THIRD_PARTY_NOTICES.txt"), parts.join("\n\n\n") + "\n");
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

const posix = (file) => path.posix.normalize(file.split("\\").join("/"));
const isSprite = (key) => key.startsWith("assets.images.") || key.startsWith("assets.sheets.");

// Packs the images and sheet images into alpha-atlas pages + alpha-atlas.json; returns
// the files still to copy (packed images are dropped unless something else needs the file).
// Filtering is per texture: each filter's images go on their own pages ("alpha-atlas-N",
// "alpha-atlas-linear-N", "alpha-atlas-sharp-N"); the manifest maps files under `filters.<filter>`.
// Sharp pages are at most SHARP_PAGE wide: the engine grows them 4x (core::kSharpScale).
const SHARP_PAGE = 1024;
function writeAtlas(root, out, files) {
  const group = (filter) => [...new Set(files.filter((f) => isSprite(f.key) && f.filter === filter).map((f) => posix(f.file)))];
  const read = (list) => new Map(list.map((f) => [f, fs.readFileSync(path.join(root, f))]));
  const prefix = (filter) => (filter === "nearest" ? "alpha-atlas" : `alpha-atlas-${filter}`);
  const atlases = Object.fromEntries(FILTERS.map((filter) =>
    [filter, buildAtlas(read(group(filter)), prefix(filter), filter === "sharp" ? SHARP_PAGE : undefined)]));
  const packedAll = new Set(FILTERS.flatMap((filter) => atlases[filter].packed));
  if (!packedAll.size) return files;
  const pages = {};
  for (const filter of FILTERS) {
    for (const p of atlases[filter].pages) {
      fs.writeFileSync(path.join(out, p.file), p.png);
      pages[p.file] = { filter };
    }
  }
  const filters = Object.fromEntries(FILTERS.map((filter) => [filter, atlases[filter].manifest.images]));
  fs.writeFileSync(path.join(out, "alpha-atlas.json"), JSON.stringify({ filters, pages }));
  const packed = (f) => atlases[f.filter].packed.includes(posix(f.file));
  const needed = new Set(files.filter((f) => !isSprite(f.key) || !packed(f)).map((f) => posix(f.file)));
  return files.filter((f) => !packedAll.has(posix(f.file)) || needed.has(posix(f.file)));
}

function copyAssets(root, out, files) {
  for (const { file } of files) {
    const dest = path.join(out, path.normalize(file));
    fs.mkdirSync(path.dirname(dest), { recursive: true });
    fs.copyFileSync(path.join(root, path.normalize(file)), dest);
  }
}

// Standalone pack: the unmodified shipping engine binary sits next to game.jsbc
// and, started with no arguments, runs it (like a fused LOVE game, nothing appended).
function engineFor(target) {
  const engine = shippingEngine(target);
  if (engine) return { engine };
  const have = describeTargets() || "(none)";
  return { problem: problem(`no shipping engine for target "${target}"; SDK has: ${have}; ` +
    "stage both flavors with tools/stage_engine.py") };
}

// The game file a pack ships: standalone packs get game.jsbc compiled from the
// staged main.js (unless bytecode is off); every other pack keeps main.js.
function gameFile(staging, standalone, { bytecode, compiler, bytecodeBits, bytecodeRunner }) {
  if (!bytecode || !(standalone.engine || bytecodeBits || bytecodeRunner)) return { file: "main.js" };
  const failed = compileBytecode({ compiler, shipping: standalone.engine ?? bytecodeRunner, bits: bytecodeBits,
                                   source: path.join(staging, "main.js"), out: path.join(staging, BYTECODE_FILE) });
  return failed ? { problem: failed } : { file: BYTECODE_FILE };
}

// root: project directory (asset paths are relative to it). target: null for
// a plain pack, or "<platform>-<arch>" to include that engine binary. atlas: pack
// images and sheet images into texture pages (default; transparent to game code).
// bytecode: standalone packs ship game.jsbc (no JS source, no source map) compiled by
// `compiler` (default: this machine's release engine). bytecodeBits: also ship game.jsbc
// without an engine binary, for a target built from source with that pointer width
// (Android), or checked against bytecodeRunner: a prebuilt engine library that will run it
// (Android prebuilt engine; not copied into the pack). shaderTool / crossTool: glslang and shadercross for assets.shaders (default: next to the release engine).
// Returns { ok, problems, files }. Writes nothing unless everything checks out.
export async function exportPack({ root, entry, out, target = null, atlas = true, bytecode = true,
                                   compiler = bundledEngine(), bytecodeBits = null, bytecodeRunner = null,
                                   shaderTool = glslangTool(), crossTool = shadercrossTool() }) {
  const standalone = target ? engineFor(target) : {};
  if (standalone.problem) return { ok: false, problems: [standalone.problem], files: [] };
  const staging = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-pack-"));
  try {
    const built = await build(entry, path.join(out, "main.js"), path.join(staging, "main.js"));
    if (!built.ok) return { ok: false, problems: built.problems, files: [] };
    const game = gameFile(staging, standalone, { bytecode, compiler, bytecodeBits, bytecodeRunner });
    if (game.problem) return { ok: false, problems: [game.problem], files: [] };
    const def = await readGameDef(entry);
    if (!def) return { ok: false, problems: [problem("main module must `export default game({...})`", entry)], files: [] };
    const { files, problems } = packFiles(root, def);
    if (problems.length) return { ok: false, problems, files };
    const shaders = path.join(staging, "shaders");
    const shaderProblems = compileShaders({ root, def, out: shaders, tool: shaderTool, crossTool });
    if (shaderProblems.length) return { ok: false, problems: shaderProblems, files };
    fs.rmSync(out, { recursive: true, force: true });
    fs.mkdirSync(out, { recursive: true });
    fs.copyFileSync(path.join(staging, game.file), path.join(out, game.file));
    copyAssets(root, out, atlas ? writeAtlas(root, out, files) : files);
    if (fs.existsSync(shaders)) fs.cpSync(shaders, out, { recursive: true });
    writeNotices(root, out, def);
    if (standalone.engine) {
      const dest = path.join(out, engineFile(target));
      fs.copyFileSync(standalone.engine, dest);
      fs.chmodSync(dest, 0o755);
    }
    return { ok: true, problems: [], files, game: game.file };
  } finally {
    fs.rmSync(staging, { recursive: true, force: true });
  }
}
