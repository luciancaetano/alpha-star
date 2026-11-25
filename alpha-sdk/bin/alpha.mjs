#!/usr/bin/env node
// alpha: the one CLI for alpha-star games. Run from the game project directory.
//
//   alpha check                  type-check only
//   alpha export [--out dist] [--standalone [--target win32-x64|linux-x64] [--no-bytecode]] [--no-atlas]
//                                type-check, bundle and copy assets into a pack (PNG images and
//                                sprite sheets packed into alpha-atlas-N.png pages unless --no-atlas);
//                                --standalone also copies the unmodified shipping engine in, so the
//                                pack runs by itself (default target: this machine), and ships the
//                                game as game.jsbc bytecode instead of main.js (--no-bytecode: main.js);
//                                --sign runs $ALPHA_SIGN_CMD ({file} = the pack's engine copy) on it
//   alpha export --target android-arm64 [--out build/android] [--engine-src <alpha-star checkout>]
//                                Gradle project (pack in its assets, engine built from source,
//                                shipping flavor); builds the APK when the Android toolchain is
//                                installed, otherwise prints what to install and the build command
//   alpha run [--watch] [--out dist] [--engine path] [-- engine args]
//                                export, then open the pack in the engine; --watch re-exports
//                                on every source/asset change and the engine hot-reloads the
//                                game (type errors are printed; the pack and game stay as they were)
//   alpha test [--entry test/main.test.ts] [--engine path]
//                                type-check + bundle the tests, run every exported
//                                test function headlessly in the engine; exit 1 on failure
//   alpha init <dir> [--template blank|platformer|topdown|puzzle|shmup] [--sdk <npm spec>]
//                                new project from a template: then npm install, npm test, npm start
//   alpha skills [--agents claude,agents,...] [--list]
//                                install or update the SDK's agent skills (e.g. alpha-harness) in
//                                each chosen agent's project folder (.claude/skills, .agents/skills, ...);
//                                without --agents it asks (Enter = claude + agents)
//   alpha --version             the SDK version (the bundled engines are built from the same version)
//   alpha mcp [--engine path]    MCP server on stdio for AI agents: check/export/test the game,
//                                start it and drive/observe it (input, step, state, screenshots, logs)
//
// Options: --entry <file> (default src/main.ts; test/main.test.ts for `test`). Engine lookup for `run`/`test`:
// --engine, $ALPHA_STAR, the engine bundled in this SDK, `alpha_star` on PATH.
import { spawn, spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { formatProblem, typecheck } from "../lib/build.mjs";
import { engineFile, hostTarget, resolveEngine } from "../lib/engine.mjs";
import { signFile } from "../lib/sign.mjs";
import { exportAndroid } from "../lib/android.mjs";
import { exportPack } from "../lib/pack.mjs";
import { buildTests, testArgs } from "../lib/test.mjs";
import { watchProject } from "../lib/watch.mjs";

const USAGE = `usage: alpha <init|check|export|run|test|mcp|skills|--version> [--entry src/main.ts] [--out dist] [--standalone [--sign]] [--no-bytecode] [--target <platform-arch>|android-arm64] [--engine-src dir] [--watch] [--no-atlas] [--engine path] [-- engine args]`;

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
    android: opt("target", "").startsWith("android") ? opt("target", "") : null,
    engineSrc: opt("engine-src", undefined),
    watch: own.includes("--watch"),
    atlas: !own.includes("--no-atlas"),
    bytecode: !own.includes("--no-bytecode"),
    sign: own.includes("--sign"),
    engineArgs: dash >= 0 ? argv.slice(dash + 1) : [],
  };
}

function report(problems, verb) {
  for (const p of problems) console.error(formatProblem(p));
  if (problems.length) console.error(`${verb} failed: ${problems.length} error(s), nothing written`);
  return problems.length ? 1 : 0;
}

async function androidCmd(args) {
  const out = args.out === path.resolve("dist") ? path.resolve("build/android") : args.out;
  const r = await exportAndroid({ root: process.cwd(), entry: args.entry, out, target: args.android,
                                  engineSrc: args.engineSrc, atlas: args.atlas, bytecode: args.bytecode });
  if (!r.ok) return report(r.problems, "android export");
  console.log(`exported Android project ${path.relative(process.cwd(), out) || "."} (${r.values.PACKAGE}, ` +
              `${r.files.length} asset file(s))`);
  console.log(r.apk ? `built ${r.apk}` : r.help);
  return 0;
}

async function exportCmd(args) {
  if (args.android) return androidCmd(args);
  const { ok, problems, files, game } = await exportPack({ root: process.cwd(), entry: args.entry, out: args.out,
                                                          target: args.target, atlas: args.atlas,
                                                          bytecode: args.bytecode });
  if (!ok) return report(problems, "export");
  if (args.sign) {
    const failed = args.target ? signFile(path.join(args.out, engineFile(args.target)))
      : { file: "", line: 0, column: 0, message: "--sign signs the engine executable: add --standalone" };
    if (failed) {
      fs.rmSync(args.out, { recursive: true, force: true });  // never leave an unsigned build that looks signed
      return report([failed], "export");
    }
  }
  const engine = args.target ? `, engine ${args.target}${args.sign ? " (signed)" : ""}` : "";
  console.log(`exported ${path.relative(process.cwd(), args.out) || "."} (${game} + ${files.length} asset file(s)${engine})`);
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

async function skillsCmd(argv) {
  const sk = await import("../lib/skills.mjs");
  const source = sk.skillsSource();
  if (!source) return report([{ file: "", line: 0, column: 0, message: "this SDK ships no skills folder" }], "skills");
  if (argv.includes("--list")) {
    console.log(`skills: ${sk.listSkills(source).join(", ")}\nagents:`);
    for (const a of sk.AGENTS) console.log(`  ${a.id.padEnd(8)} ${a.dir.padEnd(16)} ${a.label}${a.default ? " (default)" : ""}`);
    return 0;
  }
  const i = argv.indexOf("--agents");
  try {
    const agents = i >= 0 ? sk.parsePick(argv[i + 1] ?? "") : process.stdin.isTTY ? await sk.askAgents()
      : sk.parsePick("");
    for (const d of sk.installSkills({ root: process.cwd(), agents, source })) console.log(`${d.action} ${d.dir}`);
    return 0;
  } catch (e) {
    return report([{ file: "", line: 0, column: 0, message: e.message }], "skills");
  }
}

async function main(argv) {
  if (argv[0] === "--version" || argv[0] === "-v") return void console.log(VERSION) ?? 0;
  if (argv[0] === "init") return initCmd(argv);
  if (argv[0] === "skills") return skillsCmd(argv);
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
