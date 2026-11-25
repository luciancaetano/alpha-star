// `alpha test`: bundles a test module (with the game it imports) for the
// engine's headless test mode, `alpha_star --test <file>`.
import path from "node:path";
import { build } from "./build.mjs";

/** Type-checks and bundles `entry` to <root>/build/test/<name>.js. Writes nothing on errors. */
export async function buildTests({ root, entry }) {
  const outfile = path.join(root, "build", "test", `${path.basename(entry, path.extname(entry))}.js`);
  const { ok, problems } = await build(entry, outfile);
  return { ok: ok && problems.length === 0, problems, outfile };
}

/** Engine arguments for a bundled test module. */
export function testArgs(outfile) {
  return ["--test", outfile];
}
