#!/usr/bin/env node
// alpha: the one CLI for alpha-star games. Run from the game project directory.
//
//   alpha check                  type-check only
//   alpha export [--out dist] [--standalone [--target win32-x64|linux-x64]]
//                                type-check, bundle and copy assets into a pack;
//                                --standalone also copies the engine binary in,
//                                so the pack runs by itself (default target: this machine)
//   alpha run [--out dist] [--engine path] [-- engine args]
//                                export, then open the pack in the engine
//
// Options: --entry <file> (default src/main.ts). Engine lookup for `run`:
// --engine, $ALPHA_STAR, the engine bundled in this SDK, `alpha_star` on PATH.
import { spawnSync } from "node:child_process";
import path from "node:path";
import { formatProblem, typecheck } from "../lib/build.mjs";
import { hostTarget, resolveEngine } from "../lib/engine.mjs";
import { exportPack } from "../lib/pack.mjs";

const USAGE = `usage: alpha <check|export|run> [--entry src/main.ts] [--out dist] [--standalone] [--target <platform-arch>] [--engine path] [-- engine args]`;

function parse(argv) {
  const dash = argv.indexOf("--");
  const own = dash >= 0 ? argv.slice(0, dash) : argv;
  const opt = (name, fallback) => {
    const i = own.indexOf(`--${name}`);
    return i >= 0 && own[i + 1] ? own[i + 1] : fallback;
  };
  return {
    command: own[0],
    entry: path.resolve(opt("entry", "src/main.ts")),
    out: path.resolve(opt("out", "dist")),
    engine: resolveEngine(opt("engine", "")),
    target: own.includes("--standalone") ? opt("target", hostTarget()) : null,
    engineArgs: dash >= 0 ? argv.slice(dash + 1) : [],
  };
}

function report(problems, verb) {
  for (const p of problems) console.error(formatProblem(p));
  if (problems.length) console.error(`${verb} failed: ${problems.length} error(s), nothing written`);
  return problems.length ? 1 : 0;
}

async function exportCmd(args) {
  const { ok, problems, files } = await exportPack({ root: process.cwd(), entry: args.entry, out: args.out,
                                                    target: args.target });
  if (!ok) return report(problems, "export");
  const engine = args.target ? `, engine ${args.target}` : "";
  console.log(`exported ${path.relative(process.cwd(), args.out) || "."} (main.js + ${files.length} asset file(s)${engine})`);
  return 0;
}

function launch(args) {
  const run = spawnSync(args.engine, [args.out, ...args.engineArgs], { stdio: "inherit" });
  if (run.error) {
    console.error(`cannot start engine "${args.engine}": ${run.error.message}. ` +
                  `No engine bundled for ${hostTarget()}; pass --engine <path>, set ALPHA_STAR, or put alpha_star on PATH.`);
    return 1;
  }
  return run.status ?? 1;
}

async function main(argv) {
  const args = parse(argv);
  if (args.command === "check") return report(typecheck(args.entry), "check");
  if (args.command === "export") return exportCmd(args);
  if (args.command === "run") return (await exportCmd(args)) || launch(args);
  console.error(USAGE);
  return 2;
}

process.exit(await main(process.argv.slice(2)));
