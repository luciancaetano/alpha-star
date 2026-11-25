import { test } from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { bundledEngine } from "../lib/engine.mjs";
import { initProject, packageName, templates, VERSION } from "../lib/init.mjs";
import { drift } from "../scripts/sync-templates.mjs";

const CLI = path.join(path.dirname(fileURLToPath(import.meta.url)), "..", "bin", "alpha.mjs");
const alpha = (cwd, ...args) => spawnSync(process.execPath, [CLI, ...args], { cwd, encoding: "utf8" });
const tmp = () => fs.mkdtempSync(path.join(os.tmpdir(), "alpha-init-"));

test("the SDK ships the five templates", () => {
  assert.deepEqual(templates(), ["blank", "platformer", "puzzle", "shmup", "topdown"]);
});

test("templates mirror the example games (npm run templates after editing an example)", (t) => {
  if (!fs.existsSync(path.join(path.dirname(CLI), "..", "..", "examples"))) return t.skip("no examples next to this SDK");
  assert.deepEqual(drift(), []);
});

test("init writes a ready project: package name from the folder, SDK dependency, .gitignore", () => {
  const dir = path.join(tmp(), "My Game");
  const r = initProject({ dir, template: "blank" });
  assert.deepEqual(r.problems, []);
  const pkg = JSON.parse(fs.readFileSync(path.join(dir, "package.json"), "utf8"));
  assert.equal(pkg.name, "my-game");
  assert.equal(pkg.devDependencies["@alpha-star/sdk"], `^${VERSION}`);
  assert.deepEqual(Object.keys(pkg.scripts).sort(), ["check", "export", "start", "test"]);
  for (const f of [".gitignore", "AGENTS.md", "tsconfig.json", "src/main.ts", "src/types.ts", "src/alpha.ts",
                   "src/tuning.ts", "test/main.test.ts", "assets/img/player.png"]) {
    assert.ok(fs.existsSync(path.join(dir, f)), f);
  }
  assert.ok(!fs.existsSync(path.join(dir, "gitignore")));
});

test("init refuses a non-empty folder and names the templates for an unknown one", () => {
  const dir = tmp();
  fs.writeFileSync(path.join(dir, "keep.txt"), "mine");
  assert.match(initProject({ dir }).problems[0], /not empty/);
  assert.equal(fs.readFileSync(path.join(dir, "keep.txt"), "utf8"), "mine");
  assert.match(initProject({ dir: path.join(dir, "x"), template: "rpg" }).problems[0], /unknown template "rpg"; available: blank, platformer/);
});

test("package names are npm-safe", () => {
  assert.equal(packageName("/a/Cool Game!"), "cool-game-");
  assert.equal(packageName("/a/.hidden"), "hidden");
});

test("alpha --version prints the SDK version; alpha init without a folder shows usage", () => {
  assert.equal(alpha(tmp(), "--version").stdout.trim(), VERSION);
  const r = alpha(tmp(), "init");
  assert.equal(r.status, 2);
  assert.match(r.stderr, /usage: alpha init <dir>/);
});

// Every template, freshly initialized, type-checks and passes its own tests on the bundled engine.
for (const template of templates()) {
  test(`template ${template}: alpha init, then alpha check and alpha test pass`, (t) => {
    if (!bundledEngine()) return t.skip("no engine staged for this platform (python tools/stage_engine.py)");
    const root = tmp();
    assert.equal(alpha(root, "init", "game", "--template", template).status, 0);
    const game = path.join(root, "game");
    const check = alpha(game, "check");
    assert.equal(check.status, 0, check.stderr);
    const run = alpha(game, "test");
    assert.equal(run.status, 0, run.stdout + run.stderr);
    assert.match(run.stdout, / passed, 0 failed/);
    fs.rmSync(root, { recursive: true, force: true });
  });
}
