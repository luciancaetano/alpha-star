// The packed release, installed the way a user would get it: run by tools/pack_release.py
// --install-test (ALPHA_RELEASE_DIR = dist/release). Installs the local tarballs into a temp
// project (offline: npm's cache serves the third-party dependencies, the alpha-star ones are
// the files), then alpha init + npm test (alpha test) + alpha export --standalone there.
import { test } from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { hostTarget, engineFile } from "../lib/engine.mjs";

const DIR = process.env.ALPHA_RELEASE_DIR;
const tarball = (prefix) => DIR && fs.readdirSync(DIR).find((f) => f.startsWith(prefix) && f.endsWith(".tgz"));
const SDK = tarball("alpha-star-sdk-");
const ENGINE = tarball(`alpha-star-engine-${hostTarget()}-`);
const skip = !(SDK && ENGINE) && "run through python tools/pack_release.py --install-test (needs this machine's engine tarball)";

const sh = (cwd, cmd, args) => spawnSync(cmd, args, { cwd, encoding: "utf8", shell: process.platform === "win32" });
function ok(run, what) {
  assert.equal(run.status, 0, `${what} failed:\n${run.stdout}\n${run.stderr}`);
  return run.stdout;
}

test("packed tarballs install offline and run init, test and a standalone export", { skip, timeout: 600_000 }, () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-release-"));
  const sdk = path.join(DIR, SDK);
  const engine = path.join(DIR, ENGINE);
  const tool = path.join(tmp, "tool");
  fs.mkdirSync(tool);
  fs.writeFileSync(path.join(tool, "package.json"), '{ "name": "tool", "private": true }\n');
  ok(sh(tool, "npm", ["install", "--offline", "--no-audit", "--no-fund", `"${sdk}"`, `"${engine}"`]), "npm install (tool)");
  const alpha = path.join(tool, "node_modules", "@alpha-star", "sdk", "bin", "alpha.mjs");
  const version = JSON.parse(fs.readFileSync(path.join(tool, "node_modules", "@alpha-star", "sdk", "package.json"))).version;
  assert.equal(ok(sh(tool, "node", [alpha, "--version"]), "alpha --version").trim(), version);
  assert.equal(fs.existsSync(path.join(tool, "node_modules", "@alpha-star", "sdk", "engine")), false);

  ok(sh(tmp, "node", [alpha, "init", "game", "--sdk", `"file:${sdk.split(path.sep).join("/")}"`]), "alpha init");
  const game = path.join(tmp, "game");
  ok(sh(game, "npm", ["install", "--offline", "--no-audit", "--no-fund", `"${engine}"`]), "npm install (game)");
  const pkg = path.join(game, "node_modules", "@alpha-star", `engine-${hostTarget()}`);
  const said = ok(sh(game, `"${path.join(pkg, engineFile(hostTarget()))}"`, ["--version"]), "engine --version");
  assert.equal(said.trim(), `alpha-star ${version}`);
  assert.match(ok(sh(game, "npm", ["test"]), "npm test"), /PASS/);
  ok(sh(game, "npx", ["alpha", "export", "--standalone", "--out", "build/game"]), "alpha export --standalone");
  const out = path.join(game, "build", "game");
  assert.ok(fs.existsSync(path.join(out, "game.jsbc")));
  assert.equal(fs.existsSync(path.join(out, "main.js")), false);
  // The standalone engine is the platform package's shipping engine, byte for byte.
  assert.deepEqual(fs.readFileSync(path.join(out, engineFile(hostTarget()))),
                   fs.readFileSync(path.join(pkg, "shipping", engineFile(hostTarget()))));
  fs.rmSync(tmp, { recursive: true, force: true });
});
