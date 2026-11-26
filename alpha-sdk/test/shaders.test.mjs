import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { exportPack } from "../lib/pack.mjs";
import { compileShaders, compilerMessage, shaderFiles } from "../lib/shaders.mjs";

const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..", "..");
const exe = (name) => path.join(REPO, "build", "dev", "src", "Debug", name + (process.platform === "win32" ? ".exe" : ""));
const BUILT = exe("glslang");
const CROSS = exe("shadercross");
const FIXTURE = path.join(REPO, "tests", "fixtures", "shaders", "invert.frag");

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

const ASSETS = `{ sounds: {}, shaders: { invert: { file: "fx/invert.frag", uniforms: { amount: 1 } } } }`;

// A stand-in compiler: writes the output's extension ("spv", "msl") into the -o file.
const fakeRun = (calls) => (tool, args) => {
  calls.push([tool, ...args]);
  const to = args[args.indexOf("-o") + 1];
  fs.writeFileSync(to, path.extname(to).slice(1));
  return { status: 0, stdout: "", stderr: "" };
};

const FRAG = "#version 450\nlayout(location = 0) out vec4 o_color;\nvoid main() { o_color = vec4(1.0); }\n";

test("export compiles each shader to .spv and .msl in the pack and leaves the .frag out", async () => {
  const p = project(ASSETS, { "fx/invert.frag": FRAG });
  const calls = [];
  const tool = path.join(p.root, "glslang");
  const crossTool = path.join(p.root, "shadercross");
  const r = await exportPack({ ...p, bytecode: false, shaderTool: tool, crossTool: null });
  assert.equal(r.ok, false, "the real spawn of a missing tool fails");
  const def = { assets: { shaders: { invert: { file: "fx/invert.frag" } } } };
  assert.deepEqual(compileShaders({ root: p.root, def, out: p.out, tool, crossTool, run: fakeRun(calls) }), []);
  // glslang: GLSL -> SPIR-V; then shadercross: that SPIR-V -> MSL (no DXC involved).
  assert.equal(calls.length, 2);
  assert.deepEqual(calls[0].slice(0, 8), [tool, "-V", "--target-env", "vulkan1.0", "-S", "frag", "-e", "main"]);
  const spv = path.join(p.out, "fx", "invert.spv");
  assert.deepEqual(calls[1], [crossTool, spv, "-s", "SPIRV", "-d", "MSL", "-t", "fragment", "-o", path.join(p.out, "fx", "invert.msl")]);
  assert.deepEqual(fs.readdirSync(path.join(p.out, "fx")).sort(), ["invert.msl", "invert.spv"]);
});

test("without shadercross the pack keeps SPIR-V only (Vulkan still runs it)", () => {
  const p = project(ASSETS, { "fx/invert.frag": FRAG });
  const calls = [];
  const def = { assets: { shaders: { invert: { file: "fx/invert.frag" } } } };
  assert.deepEqual(compileShaders({ root: p.root, def, out: p.out, tool: "glslang", crossTool: null, run: fakeRun(calls) }), []);
  assert.equal(calls.length, 1);
  assert.deepEqual(fs.readdirSync(path.join(p.out, "fx")), ["invert.spv"]);
});

test("shader problems name the asset, the file and the line", () => {
  const p = project(ASSETS);
  const def = { assets: { shaders: { invert: { file: "fx/invert.frag" } } } };
  assert.match(compileShaders({ root: p.root, def, out: p.out, tool: "x" })[0].message,
               /assets\.shaders\.invert: "fx\/invert\.frag" is not a \.frag file inside the project/);
  fs.mkdirSync(path.join(p.root, "fx"));
  fs.writeFileSync(path.join(p.root, "fx", "invert.frag"), "x");
  assert.match(compileShaders({ root: p.root, def, out: p.out, tool: null })[0].message, /no glslang/);
  const src = path.join(p.root, "fx/invert.frag");
  const failing = () => ({ status: 1, stdout: `${src}\nERROR: ${src}:3: 'nope' : undeclared identifier\nERROR: 1 compilation errors.\n`, stderr: "" });
  assert.equal(compileShaders({ root: p.root, def, out: p.out, tool: "x", crossTool: null, run: failing })[0].message,
               "assets.shaders.invert: fx/invert.frag did not compile:\nERROR: fx/invert.frag:3: 'nope' : undeclared identifier");
  assert.equal(compilerMessage("cannot start\n", "a", "b"), "cannot start");
  assert.deepEqual(shaderFiles({}), []);
  assert.deepEqual(compileShaders({ root: p.root, def: {}, out: p.out, tool: null }), []);
});

test("export with the built glslang writes real SPIR-V; errors carry file and line", { skip: !fs.existsSync(BUILT) }, async () => {
  const p = project(ASSETS, { "fx/invert.frag": fs.readFileSync(FIXTURE, "utf8") });
  const crossTool = fs.existsSync(CROSS) ? CROSS : null;
  const r = await exportPack({ ...p, bytecode: false, shaderTool: BUILT, crossTool });
  assert.deepEqual(r.problems, []);
  const spv = fs.readFileSync(path.join(p.out, "fx", "invert.spv"));
  assert.equal(spv.readUInt32LE(0), 0x07230203);
  if (crossTool) assert.match(fs.readFileSync(path.join(p.out, "fx", "invert.msl"), "utf8"), /fragment main0_out main0\(/);
  fs.writeFileSync(path.join(p.root, "fx", "invert.frag"), "#version 450\nlayout(location = 0) out vec4 o;\nvoid main() { o = nope; }\n");
  const def = { assets: { shaders: { invert: { file: "fx/invert.frag" } } } };
  const [bad] = compileShaders({ root: p.root, def, out: p.out, tool: BUILT, crossTool: null });
  assert.match(bad.message, /fx\/invert\.frag:3: 'nope' : undeclared identifier/);
});
