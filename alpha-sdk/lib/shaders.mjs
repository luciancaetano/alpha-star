// Custom shaders (assets.shaders) in packs: each Vulkan GLSL fragment shader (.frag)
// is compiled to SPIR-V by glslang, then that SPIR-V to MSL by SDL_shadercross (both
// tools staged next to the SDK's release engine; shadercross is built without DXC),
// written next to where the source sits:
//   shaders/wave.frag -> <out>/shaders/wave.spv (Vulkan) + <out>/shaders/wave.msl (Metal)
// The .frag itself is not packed: shipping engines only load blobs. SHADER_FORMATS is the
// one place that lists blob formats (core::ShaderFormat on the engine side); a D3D12/Xbox
// format would be one more entry made from the SPIR-V (shadercross -d HLSL, then that
// platform's compiler).
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { bundledEngine } from "./engine.mjs";

const problem = (message, file = "") => ({ file, line: 0, column: 0, message });
export const SHADER_FORMATS = [{ name: "SPIR-V", ext: ".spv" }, { name: "MSL", ext: ".msl", cross: "MSL" }];

// A tool beside an engine binary (release engine by default), or null.
function toolBeside(name, engine) {
  if (!engine) return null;
  const tool = path.join(path.dirname(engine), name + (process.platform === "win32" ? ".exe" : ""));
  return fs.existsSync(tool) ? tool : null;
}
export const glslangTool = (engine = bundledEngine()) => toolBeside("glslang", engine);
export const shadercrossTool = (engine = bundledEngine()) => toolBeside("shadercross", engine);

// [{ key, file }] for every assets.shaders entry (file: the .frag, project-relative).
export function shaderFiles(def) {
  return Object.entries(def?.assets?.shaders ?? {})
    .map(([name, s]) => ({ key: `assets.shaders.${name}`, file: typeof s?.file === "string" ? s.file : "" }));
}

const runTool = (tool, args) => spawnSync(tool, args, { encoding: "utf8" });

// glslang's ERROR/WARNING lines with the compiled path shown as the project-relative
// file ("ERROR: fx/wave.frag:3: 'nope' : undeclared identifier"); the whole log otherwise.
export function compilerMessage(log, source, file) {
  const lines = log.split(/\r?\n/).filter((l) => /^(ERROR|WARNING): /.test(l) && l.includes(source));
  return lines.length ? lines.map((l) => l.replace(source, file)).join("\n") : log.trim();
}

// [tool, args] that write `to` in `format`: SPIR-V from the source (glslang), the others from
// that SPIR-V (shadercross). null when the format's tool is missing (the pack keeps the rest).
function step(format, { tool, crossTool, source, spv, to }) {
  if (!format.cross) return [tool, ["-V", "--target-env", "vulkan1.0", "-S", "frag", "-e", "main", "-o", to, source]];
  return crossTool ? [crossTool, [spv, "-s", "SPIRV", "-d", format.cross, "-t", "fragment", "-o", to]] : null;
}

// Compiles every shader of `def` from `root` into `out`. run(tool, args) -> { status, stdout, stderr }
// (spawnSync by default; tests swap it). crossTool null: SPIR-V only. Returns problems (empty when all compiled).
export function compileShaders({ root, def, out, tool = glslangTool(), crossTool = shadercrossTool(), run = runTool }) {
  const shaders = shaderFiles(def);
  if (!shaders.length) return [];
  const bad = shaders.find((s) => !/\.frag$/i.test(s.file) || !fs.existsSync(path.join(root, s.file)));
  if (bad) return [problem(`${bad.key}: "${bad.file}" is not a .frag file inside the project`)];
  if (!tool) return [problem("assets.shaders: no glslang next to the SDK's release engine; stage it with tools/stage_engine.py")];
  for (const { key, file } of shaders) {
    const source = path.join(root, file);
    const spv = path.join(out, file.replace(/\.frag$/i, ".spv"));
    for (const format of SHADER_FORMATS) {
      const to = path.join(out, file.replace(/\.frag$/i, format.ext));
      const command = step(format, { tool, crossTool, source, spv, to });
      if (!command) continue;
      fs.mkdirSync(path.dirname(to), { recursive: true });
      const r = run(...command);
      if (r.error || r.status !== 0 || !fs.existsSync(to)) {
        const log = (r.stdout ?? "") + (r.stderr ?? "") + (r.error ? String(r.error) : "");
        return [problem(`${key}: ${file} did not compile:\n${compilerMessage(log, source, file)}`, file)];
      }
    }
  }
  return [];
}
