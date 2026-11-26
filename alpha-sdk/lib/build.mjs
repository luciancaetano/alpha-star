// Compiles an alpha-star game: type-check, then bundle into one ES module.
// Engine modules ("alpha", "alpha/*", "alpha:net") stay external; the engine provides them.
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import ts from "typescript";
import * as esbuild from "esbuild";

const SDK_DIR = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const TYPES = path.join(SDK_DIR, "types", "alpha.d.ts");
const TSCONFIG = path.join(SDK_DIR, "tsconfig.game.json");
const ENGINE_MODULES = ["alpha", "alpha/*", "alpha:net"];
const TARGET = "es2023"; // QuickJS-ng feature level

/** @returns {{ file: string, line: number, column: number, message: string }[]} */
export function typecheck(entry) {
  const { config } = ts.readConfigFile(TSCONFIG, ts.sys.readFile);
  const { options } = ts.parseJsonConfigFileContent(config, ts.sys, SDK_DIR);
  const program = ts.createProgram([path.resolve(entry), TYPES], options);
  return ts.getPreEmitDiagnostics(program).map(toProblem);
}

function toProblem(d) {
  const message = ts.flattenDiagnosticMessageText(d.messageText, "\n");
  if (!d.file) return { file: "", line: 0, column: 0, message };
  const pos = d.file.getLineAndCharacterOfPosition(d.start ?? 0);
  return { file: d.file.fileName, line: pos.line + 1, column: pos.character + 1, message };
}

/**
 * Bundles without type-checking. The inline source map is relative to `outfile`
 * (where the bundle will live); the bytes are written to `writeTo` (e.g. a staging dir).
 */
export async function bundle(entry, outfile, writeTo = outfile) {
  const result = await esbuild.build({
    entryPoints: [entry],
    outfile,
    bundle: true,
    format: "esm",
    platform: "neutral",
    target: TARGET,
    external: ENGINE_MODULES,
    sourcemap: "inline",
    write: false,
    logLevel: "silent",
  });
  fs.mkdirSync(path.dirname(writeTo), { recursive: true });
  fs.writeFileSync(writeTo, result.outputFiles[0].contents);
  return result.errors.map((e) => ({
    file: e.location?.file ?? "",
    line: e.location?.line ?? 0,
    column: (e.location?.column ?? 0) + 1,
    message: e.text,
  }));
}

/** Full compile. Writes nothing if type-checking fails. */
export async function build(entry, outfile, writeTo = outfile) {
  const problems = typecheck(entry);
  if (problems.length) return { ok: false, problems };
  try {
    return { ok: true, problems: await bundle(entry, outfile, writeTo) };
  } catch (err) {
    return { ok: false, problems: fromEsbuildError(err) };
  }
}

function fromEsbuildError(err) {
  const list = err.errors ?? [{ text: String(err) }];
  return list.map((e) => ({
    file: e.location?.file ?? "",
    line: e.location?.line ?? 0,
    column: (e.location?.column ?? 0) + 1,
    message: e.text,
  }));
}

export function formatProblem(p) {
  const where = p.file ? `${path.relative(process.cwd(), p.file)}:${p.line}:${p.column}: ` : "";
  return `${where}error: ${p.message}`;
}
