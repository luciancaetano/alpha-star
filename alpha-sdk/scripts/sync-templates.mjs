// Templates for `alpha init` mirror the example games (examples/<name> is the one source of
// truth; edit there, then `npm run templates`). `blank` exists only as a template.
//   node scripts/sync-templates.mjs           copy examples -> templates
//   node scripts/sync-templates.mjs --check   exit 1 listing what differs (run by npm test)
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const SDK = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
export const EXAMPLES = path.join(SDK, "..", "examples");
export const MIRRORED = ["platformer", "topdown", "puzzle", "shmup", "shaders"];
const SKIP = new Set(["node_modules", "dist", "build", "android", "package-lock.json"]);
const renamed = (name) => (name === ".gitignore" ? "gitignore" : name);

// { relative template path -> source file } for one example.
function sources(dir, rel = "", out = {}) {
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    if (SKIP.has(e.name) || e.name.endsWith(".bat")) continue;
    const from = path.join(dir, e.name);
    const to = path.posix.join(rel, renamed(e.name));
    if (e.isDirectory()) sources(from, to, out);
    else out[to] = from;
  }
  return out;
}

function existing(dir, rel = "", out = []) {
  if (!fs.existsSync(dir)) return out;
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const to = path.posix.join(rel, e.name);
    if (e.isDirectory()) existing(path.join(dir, e.name), to, out);
    else out.push(to);
  }
  return out;
}

/** Differences between examples and templates: ["topdown: src/main.ts differs", ...]. */
export function drift() {
  const problems = [];
  for (const name of MIRRORED) {
    const want = sources(path.join(EXAMPLES, name));
    const dir = path.join(SDK, "templates", name);
    for (const [rel, from] of Object.entries(want)) {
      const to = path.join(dir, rel);
      if (!fs.existsSync(to) || !fs.readFileSync(to).equals(fs.readFileSync(from))) problems.push(`${name}: ${rel} differs`);
    }
    for (const rel of existing(dir)) if (!want[rel]) problems.push(`${name}: ${rel} is not in the example`);
  }
  return problems;
}

export function sync() {
  for (const name of MIRRORED) {
    const dir = path.join(SDK, "templates", name);
    fs.rmSync(dir, { recursive: true, force: true });
    for (const [rel, from] of Object.entries(sources(path.join(EXAMPLES, name)))) {
      fs.mkdirSync(path.dirname(path.join(dir, rel)), { recursive: true });
      fs.copyFileSync(from, path.join(dir, rel));
    }
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  if (process.argv.includes("--check")) {
    const problems = drift();
    for (const p of problems) console.error(p);
    if (problems.length) console.error("templates are stale: run `npm run templates` in alpha-sdk");
    process.exit(problems.length ? 1 : 0);
  }
  sync();
  console.log(`templates synced from examples: ${MIRRORED.join(", ")}`);
}
