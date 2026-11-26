import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { exportPack } from "../lib/pack.mjs";
import { compileShaders, shaderFiles } from "../lib/shaders.mjs";

const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const BUILT = path.join(REPO, "build", "dev", "src", "Debug", process.platform === "win32" ? "shadercross.exe" : "shadercross");
const FIXTURE = path.join(REPO, "tests", "fixtures", "shaders", "invert.hlsl");

function project(assets, files = {}) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-shader-"));
  fs.mkdirSync(path.join(root, "src"));
  fs.writeFileSync(path.join(root, "src", "main.ts"), `
import { define } from "alpha";
interface T { actions: never; sounds: never; images: never; tags: never; scenes: { play: undefined }; ui: {};
              shaders: { invert: "amount" } }
const { scene, game } = define<T>();
const play = scene("play", { enter(ctx) { ctx.postfx.add("invert", { amount: 0.5 }); } });
export default game({ config: { title: "t", size: [10, 10], seed: 1, actions: {} }, start: "play", scenes: { play },
  assets: ${assets} });
`);
  for (const [file, text] of Object.entries(files)) {
    fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
    fs.writeFileSync(path.join(root, file), text);
  }
  return { root, entry: path.join(root, "src", "main.ts"), out: path.join(root, "dist") };
}

const ASSETS = `{ sounds: {}, shaders: { invert: { file: "fx/invert.hlsl", uniforms: { amount: 1 } } } }`;

// A stand-in compiler: writes "<format>" into the -o file.
const fakeRun = (calls) => (tool, args) => {
  calls.push(args);
  fs.writeFileSync(args[args.indexOf("-o") + 1], args[args.indexOf("-d") + 1]);
  return { status: 0, stdout: "", stderr: "" };
};

test("export compiles each shader to .spv/.dxil/.msl in the pack and leaves the .hlsl out", async () => {
  const p = project(ASSETS, { "fx/invert.hlsl": "float4 main() : SV_Target { return 1; }" });
  const calls = [];
  const tool = path.join(p.root, "shadercross");
  const r = await exportPack({ ...p, bytecode: false, shaderTool: tool });
  assert.equal(r.ok, false, "the real spawn of a missing tool fails");
  const def = { assets: { shaders: { invert: { file: "fx/invert.hlsl" } } } };
  assert.deepEqual(compileShaders({ root: p.root, def, out: p.out, tool, run: fakeRun(calls) }), []);
  assert.deepEqual(calls.map((a) => a[a.indexOf("-d") + 1]), ["SPIRV", "DXIL", "MSL"]);
  assert.ok(calls.every((a) => a.includes("fragment") && a.includes("HLSL")));
  for (const [ext, fmt] of [[".spv", "SPIRV"], [".dxil", "DXIL"], [".msl", "MSL"]]) {
    assert.equal(fs.readFileSync(path.join(p.out, "fx", `invert${ext}`), "utf8"), fmt);
  }
  assert.equal(fs.existsSync(path.join(p.out, "fx", "invert.hlsl")), false);
});

test("shader problems name the asset and what is missing", () => {
  const p = project(ASSETS);
  const def = { assets: { shaders: { invert: { file: "fx/invert.hlsl" } } } };
  assert.match(compileShaders({ root: p.root, def, out: p.out, tool: "x" })[0].message,
               /assets\.shaders\.invert: "fx\/invert\.hlsl" is not an \.hlsl file inside the project/);
  fs.mkdirSync(path.join(p.root, "fx"));
  fs.writeFileSync(path.join(p.root, "fx", "invert.hlsl"), "x");
  assert.match(compileShaders({ root: p.root, def, out: p.out, tool: null })[0].message, /no shadercross/);
  const failing = () => ({ status: 1, stdout: "error: nope", stderr: "" });
  assert.match(compileShaders({ root: p.root, def, out: p.out, tool: "x", run: failing })[0].message,
               /assets\.shaders\.invert: fx\/invert\.hlsl did not compile to SPIRV:\nerror: nope/);
  assert.deepEqual(shaderFiles({}), []);
  assert.deepEqual(compileShaders({ root: p.root, def: {}, out: p.out, tool: null }), []);
});

test("export with the built shadercross writes real blobs", { skip: !fs.existsSync(BUILT) }, async () => {
  const p = project(ASSETS, { "fx/invert.hlsl": fs.readFileSync(FIXTURE, "utf8") });
  const r = await exportPack({ ...p, bytecode: false, shaderTool: BUILT });
  assert.deepEqual(r.problems, []);
  for (const ext of [".spv", ".dxil", ".msl"]) assert.ok(fs.statSync(path.join(p.out, "fx", `invert${ext}`)).size > 100);
  assert.match(fs.readFileSync(path.join(p.out, "fx", "invert.msl"), "utf8"), /fragment main0_out main0/);
});
