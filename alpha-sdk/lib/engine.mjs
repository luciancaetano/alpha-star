// Engine binaries, one folder per platform (like esbuild), in two build flavors
// (cmake/Flavors.cmake in the alpha-star repo):
//   <dir>/alpha_star[.exe]           release: `alpha run` / `alpha test` (headless flags, --test runner)
//   <dir>/shipping/alpha_star[.exe]  shipping: `alpha export --standalone` (no dev/debug/agent features)
// <dir> is looked up in this order:
//   1. the installed platform package @alpha-star/engine-<platform>-<arch> (an optionalDependency
//      of @alpha-star/sdk with the exact same version; npm installs only the one for this machine)
//   2. engine/<platform>-<arch>/ inside the SDK folder (alpha-star repo checkout: tools/stage_engine.py)
// `alpha run`/`test` then fall back to $ALPHA_STAR... see resolveEngine.
import fs from "node:fs";
import { createRequire } from "node:module";
import path from "node:path";
import { fileURLToPath } from "node:url";

const ENGINE_DIR = path.join(path.dirname(path.dirname(fileURLToPath(import.meta.url))), "engine");
const require = createRequire(import.meta.url);

// Targets published as platform packages (tools/pack_release.py builds them).
export const PLATFORM_TARGETS = ["win32-x64", "linux-x64"];
export const platformPackage = (target) => `@alpha-star/engine-${target}`;

export const hostTarget = () => `${process.platform}-${process.arch}`;

export function engineFile(target) {
  return `alpha_star${target.startsWith("win32") ? ".exe" : ""}`;
}

// The installed platform package's folder for `target`, or null.
export function packageEngineDir(target) {
  try {
    return path.dirname(require.resolve(`${platformPackage(target)}/package.json`));
  } catch {
    return null;
  }
}

function findEngine(target, ...sub) {
  const dirs = [packageEngineDir(target), path.join(ENGINE_DIR, target)].filter(Boolean);
  return dirs.map((d) => path.join(d, ...sub, engineFile(target))).find((f) => fs.existsSync(f)) ?? null;
}

export const bundledEngine = (target = hostTarget()) => findEngine(target);
export const shippingEngine = (target = hostTarget()) => findEngine(target, "shipping");

function knownTargets() {
  const local = fs.existsSync(ENGINE_DIR) ? fs.readdirSync(ENGINE_DIR) : [];
  return [...new Set([...PLATFORM_TARGETS.filter(packageEngineDir), ...local])];
}

// "win32-x64, linux-x64 (release only)": every available target and its flavors.
export function describeTargets() {
  return knownTargets()
    .filter((t) => bundledEngine(t) || shippingEngine(t))
    .map((t) => (!shippingEngine(t) ? `${t} (release only)` : !bundledEngine(t) ? `${t} (shipping only)` : t))
    .join(", ");
}

export function availableTargets() {
  return knownTargets().filter((t) => bundledEngine(t));
}

// Engine for `alpha run`: explicit path, then $ALPHA_STAR (both deliberate overrides),
// then the engine for this machine (platform package, then the SDK's engine/ folder),
// then `alpha_star` on PATH.
export function resolveEngine(explicit, env = process.env) {
  return explicit || env.ALPHA_STAR || bundledEngine() || "alpha_star";
}
