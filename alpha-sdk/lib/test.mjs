// `alpha test`: bundles a test module (with the game it imports) for the
// engine's headless test mode, `alpha_star --test <file>`.
import { spawnSync } from "node:child_process";
import path from "node:path";
import { build } from "./build.mjs";

/** Type-checks and bundles `entry` to <root>/build/test/<name>.js. Writes nothing on errors. */
export async function buildTests({ root, entry }) {
  const outfile = path.join(root, "build", "test", `${path.basename(entry, path.extname(entry))}.js`);
  const { ok, problems } = await build(entry, outfile);
  return { ok: ok && problems.length === 0, problems, outfile };
}

/** Engine arguments for a bundled test module; game assets resolve against the project root. */
export function testArgs(outfile, root) {
  return ["--test", outfile, "--assets", root];
}

/**
 * Builds and runs the tests, capturing the engine's report instead of printing it
 * (for `alpha mcp`, whose stdout is the MCP channel).
 * @returns {Promise<{ ok: boolean, problems: object[], output: string }>}
 */
export async function runTests({ root, entry, engine }) {
  const built = await buildTests({ root, entry });
  if (!built.ok) return { ok: false, problems: built.problems, output: "" };
  const run = spawnSync(engine, testArgs(built.outfile, root), { encoding: "utf8" });
  if (run.error) return { ok: false, problems: [], output: `cannot start engine "${engine}": ${run.error.message}` };
  return { ok: run.status === 0, problems: [], output: `${run.stdout}${run.stderr}` };
}
