import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { build } from "../lib/build.mjs";

const fixture = (name) => path.join(import.meta.dirname, "fixtures", name, "game.ts");
const tmpOut = () => path.join(fs.mkdtempSync(path.join(os.tmpdir(), "alpha-")), "game.js");

test("bundles local imports into one module and keeps alpha external", async () => {
  const out = tmpOut();
  const { ok, problems } = await build(fixture("ok"), out);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  const js = fs.readFileSync(out, "utf8");
  assert.match(js, /from "alpha"/);
  assert.doesNotMatch(js, /\.\/helpers/);
  assert.match(js, /export \{[^}]*default/);
});

test("rejects an undeclared action with file and line, writing nothing", async () => {
  const out = tmpOut();
  const { ok, problems } = await build(fixture("bad"), out);
  assert.equal(ok, false);
  assert.equal(problems.length, 1);
  assert.equal(problems[0].line, 13);
  assert.match(problems[0].message, /"fly".*"jump"/s);
  assert.equal(fs.existsSync(out), false);
});
