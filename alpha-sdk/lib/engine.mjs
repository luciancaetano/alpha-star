// Engine binaries shipped inside the SDK, one per platform (like Electron):
//   engine/<platform>-<arch>/alpha_star[.exe]   e.g. engine/win32-x64, engine/linux-x64
// Staged by tools/stage_engine.py in the alpha-star repo.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const ENGINE_DIR = path.join(path.dirname(path.dirname(fileURLToPath(import.meta.url))), "engine");

export const hostTarget = () => `${process.platform}-${process.arch}`;

export function engineFile(target) {
  return `alpha_star${target.startsWith("win32") ? ".exe" : ""}`;
}

export function bundledEngine(target = hostTarget()) {
  const file = path.join(ENGINE_DIR, target, engineFile(target));
  return fs.existsSync(file) ? file : null;
}

export function availableTargets() {
  if (!fs.existsSync(ENGINE_DIR)) return [];
  return fs.readdirSync(ENGINE_DIR).filter((t) => bundledEngine(t));
}

// Engine for `alpha run`: explicit path, then $ALPHA_STAR, then the bundled
// binary for this machine, then `alpha_star` on PATH.
export function resolveEngine(explicit, env = process.env) {
  return explicit || env.ALPHA_STAR || bundledEngine() || "alpha_star";
}
