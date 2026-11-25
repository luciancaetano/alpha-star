// Standalone packs ship the game as game.jsbc: QuickJS bytecode (no source, no
// debug info) compiled by the SDK's release engine (`alpha_star --compile`) and
// stamped with its build. The shipping engine next to it runs only bytecode with
// its own stamp, so both engines must come from the same staging. Not encryption:
// names and strings in the game stay readable to a determined reader.
import { spawnSync } from "node:child_process";
import fs from "node:fs";

export const BYTECODE_FILE = "game.jsbc";
const HEADER = 64;  // src/core/bytecode.hpp
const problem = (message) => ({ file: "", line: 0, column: 0, message });

// { engine, quickjs, build } from a game.jsbc header, or null when it is not one.
export function stampOf(bytes) {
  if (bytes.length < HEADER || bytes.subarray(0, 8).toString("latin1") !== "ALPHAJBC") return null;
  const text = (at, size) => bytes.subarray(at, at + size).toString("latin1").replace(/\0+$/, "");
  return { engine: text(24, 12), quickjs: text(36, 12), build: text(48, 16) };
}

// The shipping engine carries its stamp as plain strings: one that lacks the
// compiler's build id would refuse the game at start.
function mismatch(stamp, shipping) {
  const binary = fs.readFileSync(shipping);
  const ok = binary.includes(stamp.build, 0, "latin1") && binary.includes(stamp.engine, 0, "latin1");
  return ok ? null : problem(`the SDK's release engine (compiler, alpha-star ${stamp.engine}, build ${stamp.build}) ` +
    "and its shipping engine are different builds; restage both with tools/stage_engine.py");
}

// Compiles `source` (bundled main.js) to `out`. compiler: the release engine for
// this machine; shipping: the engine that will run it. Returns a problem or null.
export function compileBytecode({ compiler, shipping, source, out }) {
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
  return mismatch(stamp, shipping);
}
