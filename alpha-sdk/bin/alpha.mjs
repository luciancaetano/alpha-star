#!/usr/bin/env node
// alpha: the one CLI for alpha-star games. Run from the game project directory.
//
//   alpha check                  type-check only
//   alpha export [--out dist] [--standalone [--target win32-x64|linux-x64]] [--no-atlas]
//                                type-check, bundle and copy assets into a pack (PNG images and
//                                sprite sheets packed into alpha-atlas-N.png pages unless --no-atlas);
//                                --standalone also copies the shipping engine in,
//                                so the pack runs by itself (default target: this machine)
//   alpha run [--watch] [--out dist] [--engine path] [-- engine args]
//                                export, then open the pack in the engine; --watch re-exports
//                                on every source/asset change and the engine hot-reloads the
//                                game (type errors are printed; the pack and game stay as they were)
//   alpha test [--entry test/main.test.ts] [--engine path]
//                                type-check + bundle the tests, run every exported
//                                test function headlessly in the engine; exit 1 on failure
//   alpha init <dir> [--template blank|platformer|topdown|puzzle|shmup] [--sdk <npm spec>]
//                                new project from a template: then npm install, npm test, npm start
//   alpha --version              the SDK version (the bundled engines are built from the same version)
//   alpha mcp [--engine path]    MCP server on stdio for AI agents: check/export/test the game,
//                                start it and drive/observe it (input, step, state, screenshots, logs)
//
// Options: --entry <file> (default src/main.ts; test/main.test.ts for `test`). Engine lookup for `run`/`test`:
// --engine, $ALPHA_STAR, the engine bundled in this SDK, `alpha_star` on PATH.
import { spawn, spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { formatProblem, typecheck } from "../lib/build.mjs";
import { hostTarget, resolveEngine } from "../lib/engine.mjs";
import { exportPack } from "../lib/pack.mjs";
import { buildTests, testArgs } from "../lib/test.mjs";
import { watchProject } from "../lib/watch.mjs";

const USAGE = `usage: alpha <init|check|export|run|test|mcp|--version> [--entry src/main.ts] [--out dist] [--standalone] [--target <platform-arch>] [--watch] [--no-atlas] [--engine path] [-- engine args]`;

import { initProject, VERSION } from "../lib/init.mjs";

function parse(argv) {
  const dash = argv.indexOf("--");
  const own = dash >= 0 ? argv.slice(0, dash) : argv;
  const opt = (name, fallback) => {
    const i = own.indexOf(`--${name}`);
    return i >= 0 && own[i + 1] ? own[i + 1] : fallback;
  };
  return {
    command: own[0],
    entry: path.resolve(opt("entry", own[0] === "test" ? "test/main.test.ts" : "src/main.ts")),
    out: path.resolve(opt("out", "dist")),
    engine: resolveEngine(opt("engine", "")),
    target: own.includes("--standalone") ? opt("target", hostTarget()) : null,
    watch: own.includes("--watch"),
    atlas: !own.includes("--no-atlas"),
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
                                                    target: args.target, atlas: args.atlas });
  if (!ok) return report(problems, "export");
  const engine = args.target ? `, engine ${args.target}` : "";
  console.log(`exported ${path.relative(process.cwd(), args.out) || "."} (main.js + ${files.length} asset file(s)${engine})`);
  return 0;
}

async function testCmd(args) {
  const { ok, problems, outfile } = await buildTests({ root: process.cwd(), entry: args.entry });
  if (!ok) return report(problems, "test build");
  return launch(args, testArgs(outfile, process.cwd()));
}

function cannotStart(args, error) {
  console.error(`cannot start engine "${args.engine}": ${error.message}. ` +
                `No engine bundled for ${hostTarget()}; pass --engine <path>, set ALPHA_STAR, or put alpha_star on PATH.`);
  return 1;
}

function onExport(r) {
  if (r.ok) console.log(`re-exported (main.js + ${r.files.length} asset file(s)); the engine reloads the game`);
  else report(r.problems, "export");
}

// The engine runs the pack while every change re-exports it; ends when the game closes.
function launchWatching(args) {
  const child = spawn(args.engine, [args.out, ...args.engineArgs], { stdio: "inherit" });
  const watcher = watchProject({ root: process.cwd(), entry: args.entry, out: args.out, onExport });
  console.log("watching for changes (close the game window to stop)");
  return new Promise((resolve) => {
    const done = (code) => {
      watcher.close();
      resolve(code);
    };
    child.once("error", (e) => done(cannotStart(args, e)));
    child.once("exit", (code) => done(code ?? 1));
  });
}

async function runCmd(args) {
  const code = await exportCmd(args);
  if (code) return code;
  return args.watch ? launchWatching(args) : launch(args);
}

function launch(args, engineArgs = [args.out, ...args.engineArgs]) {
  const run = spawnSync(args.engine, engineArgs, { stdio: "inherit" });
  if (run.error) return cannotStart(args, run.error);
  return run.status ?? 1;
}

function initCmd(argv) {
  const opt = (name, fallback) => (argv.includes(`--${name}`) ? argv[argv.indexOf(`--${name}`) + 1] : fallback);
  const dir = argv[1];
  if (!dir || dir.startsWith("--")) {
    console.error("usage: alpha init <dir> [--template blank|platformer|topdown|puzzle|shmup] [--sdk <npm spec>]");
    return 2;
  }
  const r = initProject({ dir, template: opt("template", "blank"), sdk: opt("sdk", undefined) });
  if (!r.ok) return report(r.problems.map((message) => ({ file: "", line: 0, column: 0, message })), "init");
  console.log(`created ${dir} (${r.files.length} files). Next:
  cd ${dir}
  npm install
  npm test
  npm start`);
  return 0;
}

async function main(argv) {
  if (argv[0] === "--version" || argv[0] === "-v") return void console.log(VERSION) ?? 0;
  if (argv[0] === "init") return initCmd(argv);
  const args = parse(argv);
  if (args.command === "check") return report(typecheck(args.entry), "check");
  if (args.command === "export") return exportCmd(args);
  if (args.command === "run") return runCmd(args);
  if (args.command === "test") return testCmd(args);
  if (args.command === "mcp") return (await import("../lib/mcp.mjs")).runMcp({ root: process.cwd(), engine: args.engine, version: VERSION });
  console.error(USAGE);
  return 2;
}

process.exit(await main(process.argv.slice(2)));
