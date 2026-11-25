// `alpha init <dir> [--template name]`: copies a ready-to-run project from templates/<name>.
// Templates are complete small games (src, test, assets, AGENTS.md); `blank` is the smallest.
// npm strips `.gitignore` from published packages, so templates keep it as `gitignore`.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

export const SDK_DIR = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
export const TEMPLATES_DIR = path.join(SDK_DIR, "templates");
export const VERSION = JSON.parse(fs.readFileSync(path.join(SDK_DIR, "package.json"), "utf8")).version;
const RENAMED = { gitignore: ".gitignore" };

export function templates() {
  if (!fs.existsSync(TEMPLATES_DIR)) return [];
  return fs.readdirSync(TEMPLATES_DIR).filter((t) => fs.existsSync(path.join(TEMPLATES_DIR, t, "package.json"))).sort();
}

function copyTree(from, to, files) {
  fs.mkdirSync(to, { recursive: true });
  for (const entry of fs.readdirSync(from, { withFileTypes: true })) {
    const target = path.join(to, RENAMED[entry.name] ?? entry.name);
    if (entry.isDirectory()) copyTree(path.join(from, entry.name), target, files);
    else {
      fs.copyFileSync(path.join(from, entry.name), target);
      files.push(target);
    }
  }
}

// npm package names: lowercase, no spaces.
export function packageName(dir) {
  return path.basename(path.resolve(dir)).toLowerCase().replace(/[^a-z0-9._-]+/g, "-").replace(/^[._-]+/, "") || "game";
}

function rewritePackage(dir, sdk) {
  const file = path.join(dir, "package.json");
  const pkg = JSON.parse(fs.readFileSync(file, "utf8"));
  pkg.name = packageName(dir);
  pkg.devDependencies = { ...pkg.devDependencies, "@alpha-star/sdk": sdk };
  fs.writeFileSync(file, JSON.stringify(pkg, null, 2) + "\n");
}

/**
 * Creates the project. `sdk` is the dependency spec written to package.json
 * (default `^<this SDK's version>`; e.g. `file:../alpha-sdk` for a local SDK).
 * @returns {{ ok: boolean, problems: string[], files: string[] }}
 */
export function initProject({ dir, template = "blank", sdk = `^${VERSION}` }) {
  const available = templates();
  if (!available.includes(template)) {
    return { ok: false, problems: [`unknown template "${template}"; available: ${available.join(", ")}`], files: [] };
  }
  const root = path.resolve(dir);
  if (fs.existsSync(root) && fs.readdirSync(root).length) {
    return { ok: false, problems: [`"${dir}" exists and is not empty; pick a new folder`], files: [] };
  }
  const files = [];
  copyTree(path.join(TEMPLATES_DIR, template), root, files);
  rewritePackage(root, sdk);
  return { ok: true, problems: [], files };
}
