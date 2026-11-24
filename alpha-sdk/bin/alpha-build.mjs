#!/usr/bin/env node
// Usage: alpha-build <entry.ts> [-o out.js]
import path from "node:path";
import { build, formatProblem } from "../lib/build.mjs";

const USAGE = "usage: alpha-build <entry.ts> [-o <out.js>]";

function parseArgs(argv) {
  const entry = argv.find((a, i) => !a.startsWith("-") && argv[i - 1] !== "-o");
  const o = argv.indexOf("-o");
  if (!entry) return null;
  const outfile = o >= 0 ? argv[o + 1] : path.join("dist", path.basename(entry).replace(/\.[cm]?tsx?$/, ".js"));
  return outfile ? { entry, outfile } : null;
}

const args = parseArgs(process.argv.slice(2));
if (!args) {
  console.error(USAGE);
  process.exit(2);
}
const { ok, problems } = await build(args.entry, args.outfile);
for (const p of problems) console.error(formatProblem(p));
if (!ok) {
  console.error(`build failed: ${problems.length} error(s), nothing written`);
  process.exit(1);
}
console.log(`built ${args.outfile}`);
