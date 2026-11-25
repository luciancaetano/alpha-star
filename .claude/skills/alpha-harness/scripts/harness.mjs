#!/usr/bin/env node
// alpha-harness: copy the SDK's LLM docs into <project>/docs/llm and write the managed
// block of <project>/AGENTS.md (docs index + code quality rules). Idempotent: rerun after
// every SDK update. Text outside the managed block is never touched.
//
//   node harness.mjs [projectDir] [--sdk <path to @alpha-star/sdk>]
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const START = "<!-- alpha-harness:start (generated; edit outside this block) -->";
const END = "<!-- alpha-harness:end -->";

function parseArgs(argv) {
  const out = { project: process.cwd(), sdk: null };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--sdk") out.sdk = path.resolve(argv[++i]);
    else out.project = path.resolve(argv[i]);
  }
  return out;
}

// The SDK's llm/ folder: --sdk, the project's node_modules, or the repo checkout this skill sits in.
function findDocs(project, sdk) {
  const here = path.dirname(fileURLToPath(import.meta.url));
  const candidates = [
    sdk && path.join(sdk, "llm"),
    path.join(project, "node_modules", "@alpha-star", "sdk", "llm"),
    path.resolve(here, "..", "..", "..", "alpha-sdk", "llm"),
  ].filter(Boolean);
  const found = candidates.find((c) => fs.existsSync(path.join(c, "README.md")));
  if (!found) throw new Error(`alpha-harness: no LLM docs found. Looked in:\n  ${candidates.join("\n  ")}\n` +
    "Run `npm install` in the project (needs @alpha-star/sdk) or pass --sdk <path>.");
  return found;
}

function summaryOf(text, file) {
  const m = text.match(/^> summary:\s*(.+)$/m);
  return m ? m[1].trim() : `(no summary line in ${file})`;
}

function copyDocs(src, dest) {
  fs.mkdirSync(dest, { recursive: true });
  const files = fs.readdirSync(src).filter((f) => f.endsWith(".md")).sort();
  const index = [];
  for (const f of files) {
    const text = fs.readFileSync(path.join(src, f), "utf8");
    fs.writeFileSync(path.join(dest, f), text);
    if (f !== "README.md") index.push({ file: f, summary: summaryOf(text, f) });
  }
  return index;
}

function block(index, rules) {
  const rows = index.map((d) => `| [docs/llm/${d.file}](docs/llm/${d.file}) | ${d.summary} |`);
  return [START, "", "## alpha-star API docs (read on demand)", "",
    "This game runs on the alpha-star engine. Do not guess the API: before using a feature,",
    "read the matching doc below (they are short). `node_modules/@alpha-star/sdk/types/alpha.d.ts`",
    "is the full typed reference. These files are copied from the SDK by the alpha-harness skill;",
    "rerun it after updating `@alpha-star/sdk` instead of editing them.", "",
    "| Doc | Read it when |", "|---|---|", ...rows, "", rules.trim(), "", END].join("\n");
}

function writeAgents(project, content) {
  const file = path.join(project, "AGENTS.md");
  const old = fs.existsSync(file) ? fs.readFileSync(file, "utf8") : `# ${path.basename(project)}\n`;
  const s = old.indexOf(START), e = old.indexOf(END);
  const next = s >= 0 && e > s
    ? old.slice(0, s) + content + old.slice(e + END.length)
    : `${old.trimEnd()}\n\n${content}\n`;
  fs.writeFileSync(file, next);
  return file;
}

const args = parseArgs(process.argv.slice(2));
const docs = findDocs(args.project, args.sdk);
const index = copyDocs(docs, path.join(args.project, "docs", "llm"));
const here = path.dirname(fileURLToPath(import.meta.url));
const rules = fs.readFileSync(path.join(here, "..", "references", "quality-rules.md"), "utf8");
const file = writeAgents(args.project, block(index, rules));
console.log(`alpha-harness: ${index.length} docs -> ${path.join(args.project, "docs", "llm")}; updated ${file}`);
