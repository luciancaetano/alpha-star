import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { availableTargets, bundledEngine, engineFile, hostTarget, resolveEngine } from "../lib/engine.mjs";
import { exportPack } from "../lib/pack.mjs";

const ENGINE_DIR = path.join(path.dirname(path.dirname(fileURLToPath(import.meta.url))), "engine");
const FAKE = "testos-x99";  // never a real target, so real staged engines stay untouched

function withFakeEngine(fn) {
  const dir = path.join(ENGINE_DIR, FAKE);
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, engineFile(FAKE)), "ENGINE");
  return Promise.resolve(fn()).finally(() => fs.rmSync(dir, { recursive: true, force: true }));
}

test("target names follow node: platform-arch; .exe only on win32", () => {
  assert.equal(hostTarget(), `${process.platform}-${process.arch}`);
  assert.equal(engineFile("win32-x64"), "alpha_star.exe");
  assert.equal(engineFile("linux-x64"), "alpha_star");
});

test("engine lookup: explicit, then ALPHA_STAR, then bundled, then PATH", () => {
  assert.equal(resolveEngine("/x/engine", { ALPHA_STAR: "/y" }), "/x/engine");
  assert.equal(resolveEngine("", { ALPHA_STAR: "/y" }), "/y");
  assert.equal(resolveEngine("", {}), bundledEngine() ?? "alpha_star");
});

test("bundled engines are discovered per target", () =>
  withFakeEngine(() => {
    assert.ok(availableTargets().includes(FAKE));
    assert.equal(bundledEngine(FAKE), path.join(ENGINE_DIR, FAKE, engineFile(FAKE)));
    assert.equal(bundledEngine("nope-x0"), null);
  }));

function project() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-std-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: never; sounds: never; images: never; tags: never; scenes: { play: undefined }; ui: {} }
const { scene, game } = define<T>();
export default game({ config: { title: "t", size: [10, 10], seed: 1, actions: {} }, start: "play",
  scenes: { play: scene("play", {}) }, assets: { sounds: {} } });
`);
  return { root, entry: path.join(root, "src", "main.ts"), out: path.join(root, "dist") };
}

test("standalone export puts the target's engine next to main.js", () =>
  withFakeEngine(async () => {
    const p = project();
    const { ok, problems } = await exportPack({ ...p, target: FAKE });
    assert.deepEqual(problems, []);
    assert.ok(ok);
    assert.equal(fs.readFileSync(path.join(p.out, engineFile(FAKE)), "utf8"), "ENGINE");
    assert.ok(fs.existsSync(path.join(p.out, "main.js")));
  }));

test("standalone export for a missing target lists what the SDK has", async () => {
  const p = project();
  const { ok, problems } = await exportPack({ ...p, target: "nope-x0" });
  assert.equal(ok, false);
  assert.match(problems[0].message, /no bundled engine for target "nope-x0"; SDK has:/);
  assert.equal(fs.existsSync(p.out), false);
});
