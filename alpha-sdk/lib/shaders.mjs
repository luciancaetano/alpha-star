// Custom shaders (assets.shaders) in packs: each HLSL fragment shader is compiled by
// SDL_shadercross (the `shadercross` tool staged next to the SDK's release engine)
// into one blob per GPU backend, written next to where the source sits:
//   shaders/wave.hlsl -> <out>/shaders/wave.spv (Vulkan), .dxil (Direct3D 12), .msl (Metal)
// The .hlsl itself is not packed: shipping engines only load blobs.
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { bundledEngine } from "./engine.mjs";

const problem = (message, file = "") => ({ file, line: 0, column: 0, message });
export const SHADER_FORMATS = [["SPIRV", ".spv"], ["DXIL", ".dxil"], ["MSL", ".msl"]];

// The shadercross tool beside an engine binary (release engine by default), or null.
export function shadercrossTool(engine = bundledEngine()) {
  if (!engine) return null;
  const tool = path.join(path.dirname(engine), process.platform === "win32" ? "shadercross.exe" : "shadercross");
  return fs.existsSync(tool) ? tool : null;
}

// [{ key, file }] for every assets.shaders entry (file: the .hlsl, project-relative).
export function shaderFiles(def) {
  return Object.entries(def?.assets?.shaders ?? {})
    .map(([name, s]) => ({ key: `assets.shaders.${name}`, file: typeof s?.file === "string" ? s.file : "" }));
}

const runTool = (tool, args) => spawnSync(tool, args, { encoding: "utf8" });

// Compiles every shader of `def` from `root` into `out`. run(tool, args) -> { status, stdout, stderr }
// (spawnSync by default; tests swap it). Returns problems (empty when all compiled).
export function compileShaders({ root, def, out, tool = shadercrossTool(), run = runTool }) {
  const shaders = shaderFiles(def);
  if (!shaders.length) return [];
  const bad = shaders.find((s) => !/\.hlsl$/i.test(s.file) || !fs.existsSync(path.join(root, s.file)));
  if (bad) return [problem(`${bad.key}: "${bad.file}" is not an .hlsl file inside the project`)];
  if (!tool) return [problem("assets.shaders: no shadercross next to the SDK's release engine; stage it with tools/stage_engine.py")];
  for (const { key, file } of shaders) {
    for (const [dest, ext] of SHADER_FORMATS) {
      const to = path.join(out, file.replace(/\.hlsl$/i, ext));
      fs.mkdirSync(path.dirname(to), { recursive: true });
      const r = run(tool, [path.join(root, file), "-s", "HLSL", "-d", dest, "-t", "fragment", "-o", to]);
      if (r.error || r.status !== 0 || !fs.existsSync(to)) {
        return [problem(`${key}: ${file} did not compile to ${dest}:\n${(r.stdout ?? "") + (r.stderr ?? String(r.error ?? ""))}`.trim(), file)];
      }
    }
  }
  return [];
}
