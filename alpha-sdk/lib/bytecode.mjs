// Standalone packs ship the game as game.jsbc: QuickJS bytecode (no source, no
// debug info) compiled by the SDK's release engine (`alpha_star --compile`) and
// stamped with its build, encrypted (XChaCha20-Poly1305) with a key of that engine
// build. The shipping engine next to it runs only bytecode with its own stamp and key,
// so both engines must come from the same staging. This stops casual extraction
// (strings, decompilers on a plain file), not a determined attacker: the key ships
// inside the engine.
import { spawnSync } from "node:child_process";
import fs from "node:fs";

export const BYTECODE_FILE = "game.jsbc";
const HEADER = 112;  // src/core/bytecode.hpp
const problem = (message) => ({ file: "", line: 0, column: 0, message });

// { engine, quickjs, build, key } from a game.jsbc header (key: the engine key's
// public id), or null when it is not one.
export function stampOf(bytes) {
  if (bytes.length < HEADER || bytes.subarray(0, 8).toString("latin1") !== "ALPHAJBC") return null;
  const text = (at, size) => bytes.subarray(at, at + size).toString("latin1").replace(/\0+$/, "");
  return { engine: text(72, 12), quickjs: text(84, 12), build: text(96, 16), key: text(16, 16) };
}

// The shipping engine carries its stamp and key id as plain strings: one that lacks
// the compiler's build or key id would refuse the game at start.
function mismatch(stamp, shipping) {
  const binary = fs.readFileSync(shipping);
  const ok = [stamp.build, stamp.engine, stamp.key].every((s) => binary.includes(s, 0, "latin1"));
  return ok ? null : problem(`the SDK's release engine (compiler, alpha-star ${stamp.engine}, build ${stamp.build}, key ${stamp.key}) ` +
    `and the engine that will run it (${shipping}) are different builds: install @alpha-star/sdk and its engine ` +
    "packages at one version (npm install), or in a checkout restage with tools/stage_engine.py / tools/stage_android.py");
}

// A target built from source (Android): its build id is the same QuickJS source hash
// as this checkout's, plus its pointer width. QuickJS-ng writes bytecode little-endian
// with LEB128 counts on every host (quickjs.c bc_put_*), so only the width must match.
function widthMismatch(stamp, bits) {
  return stamp.build.endsWith(`-${bits}`) ? null
    : problem(`the SDK's release engine compiles for build ${stamp.build}, but the target needs ${bits}-bit ` +
              "pointers; export from a 64-bit machine, or with --no-bytecode (ships main.js)");
}

// Compiles `source` (bundled main.js) to `out`. compiler: the release engine for
// this machine; shipping: the engine that will run it, or bits: the pointer width of
// a target built from source. Returns a problem or null.
export function compileBytecode({ compiler, shipping, bits, source, out }) {
  if (!compiler) {
    return problem("no release engine for this machine to compile game.jsbc; stage both flavors with " +
                   "tools/stage_engine.py, or export with --no-bytecode (ships main.js)");
  }
  const run = spawnSync(compiler, ["--compile", source, out], { encoding: "utf8" });
  if (run.error || run.status !== 0) {
    return problem(`compiling game.jsbc with ${compiler} failed: ${(run.stderr || String(run.error ?? "")).trim()}`);
  }
  const stamp = stampOf(fs.readFileSync(out));
  if (!stamp) return problem(`${compiler} wrote no game.jsbc header (is it an alpha-star release engine?)`);
  return shipping ? mismatch(stamp, shipping) : widthMismatch(stamp, bits);
}
