import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { buildTests, testArgs } from "../lib/test.mjs";

function project(testSource) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-test-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.mkdirSync(path.join(root, "test"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: "jump"; sounds: never; images: never; tags: { x: {} }; scenes: { play: undefined }; ui: {} }
const { scene, game } = define<T>();
export default game({ config: { title: "t", size: [10, 10], seed: 1, actions: { jump: ["Space"] } },
  start: "play", scenes: { play: scene("play", {}) }, assets: { sounds: {} } });
`);
  fs.writeFileSync(path.join(root, "test", "main.test.ts"), testSource);
  return { root, entry: path.join(root, "test", "main.test.ts") };
}

test("buildTests bundles the test file with the game, keeping alpha modules external", async () => {
  const p = project(`import { simulate } from "alpha/test";
import game from "../src/main";
export function staysInPlay() { simulate(game, { seconds: 1, inputs: { jump: [0] } }).expect.scene("play"); }
`);
  const { ok, problems, outfile } = await buildTests(p);
  assert.deepEqual(problems, []);
  assert.ok(ok);
  assert.equal(outfile, path.join(p.root, "build", "test", "main.test.js"));
  const js = fs.readFileSync(outfile, "utf8");
  assert.match(js, /from "alpha\/test"/);
  assert.match(js, /from "alpha"/);
  assert.match(js, /sourceMappingURL=data:application\/json;base64,/);
});

test("buildTests refuses tests that do not type-check, naming the line", async () => {
  const p = project(`import { simulate } from "alpha/test";
import game from "../src/main";
export function t() { simulate(game, { seconds: 1, inputs: { fly: [0] } }); }
`);
  const { ok, problems } = await buildTests(p);
  assert.equal(ok, false);
  assert.equal(problems[0].line, 3);
  assert.match(problems[0].message, /fly/);
});

test("the engine runs tests in --test mode", () => {
  assert.deepEqual(testArgs("/p/build/test/main.test.js", "/p"), ["--test", "/p/build/test/main.test.js", "--assets", "/p"]);
});
