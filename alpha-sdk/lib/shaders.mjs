// Custom shaders (assets.shaders) in packs: each Vulkan GLSL fragment shader (.frag)
// is compiled to SPIR-V by glslang (the `glslang` tool staged next to the SDK's
// release engine), written next to where the source sits:
//   shaders/wave.frag -> <out>/shaders/wave.spv
// The .frag itself is not packed: shipping engines only load .spv.
// SHADER_FORMATS is the one place that lists blob formats: SPIR-V (Vulkan) only today;
// Metal (SPIR-V -> MSL through SPIRV-Cross at export) is planned.
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { bundledEngine } from "./engine.mjs";

const problem = (message, file = "") => ({ file, line: 0, column: 0, message });
export const SHADER_FORMATS = [{ name: "SPIR-V", ext: ".spv" }];

// The glslang tool beside an engine binary (release engine by default), or null.
export function glslangTool(engine = bundledEngine()) {
  if (!engine) return null;
  const tool = path.join(path.dirname(engine), process.platform === "win32" ? "glslang.exe" : "glslang");
  return fs.existsSync(tool) ? tool : null;
}

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

// Compiles every shader of `def` from `root` into `out`. run(tool, args) -> { status, stdout, stderr }
// (spawnSync by default; tests swap it). Returns problems (empty when all compiled).
export function compileShaders({ root, def, out, tool = glslangTool(), run = runTool }) {
  const shaders = shaderFiles(def);
  if (!shaders.length) return [];
  const bad = shaders.find((s) => !/\.frag$/i.test(s.file) || !fs.existsSync(path.join(root, s.file)));
  if (bad) return [problem(`${bad.key}: "${bad.file}" is not a .frag file inside the project`)];
  if (!tool) return [problem("assets.shaders: no glslang next to the SDK's release engine; stage it with tools/stage_engine.py")];
  for (const { key, file } of shaders) {
    const source = path.join(root, file);
    for (const { ext } of SHADER_FORMATS) {
      const to = path.join(out, file.replace(/\.frag$/i, ext));
      fs.mkdirSync(path.dirname(to), { recursive: true });
      const r = run(tool, ["-V", "--target-env", "vulkan1.0", "-S", "frag", "-e", "main", "-o", to, source]);
      if (r.error || r.status !== 0 || !fs.existsSync(to)) {
        const log = (r.stdout ?? "") + (r.stderr ?? "") + (r.error ? String(r.error) : "");
        return [problem(`${key}: ${file} did not compile:\n${compilerMessage(log, source, file)}`, file)];
      }
    }
  }
  return [];
}
