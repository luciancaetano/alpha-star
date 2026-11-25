// Engine binaries shipped inside the SDK, one per platform (like Electron), in two
// build flavors (cmake/Flavors.cmake in the alpha-star repo):
//   engine/<platform>-<arch>/alpha_star[.exe]           release: `alpha run` / `alpha test`
//                                                       (headless flags, --test runner)
//   engine/<platform>-<arch>/shipping/alpha_star[.exe]  shipping: `alpha export --standalone`
//                                                       (no dev/debug/agent features, no console)
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

export function shippingEngine(target = hostTarget()) {
  const file = path.join(ENGINE_DIR, target, "shipping", engineFile(target));
  return fs.existsSync(file) ? file : null;
}

// "win32-x64, linux-x64 (release only)": every staged target and its flavors.
export function describeTargets() {
  if (!fs.existsSync(ENGINE_DIR)) return "";
  return fs.readdirSync(ENGINE_DIR)
    .filter((t) => bundledEngine(t) || shippingEngine(t))
    .map((t) => (!shippingEngine(t) ? `${t} (release only)` : !bundledEngine(t) ? `${t} (shipping only)` : t))
    .join(", ");
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
