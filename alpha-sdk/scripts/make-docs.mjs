// Builds the static docs site: docs/guide/*.md (hand-written) + an API reference generated
// from types/alpha.d.ts with the TypeScript compiler API -> docs/site/*.html. No CDN, no
// required scripts: the pages work opened straight from disk; landing copy is optional.
//   node scripts/make-docs.mjs           write docs/site/
//   node scripts/make-docs.mjs --check   exit 1 if docs/site/ is not what the sources make (npm test)
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import ts from "typescript";
import { decodePng, encodePng } from "../lib/png.mjs";
import { landing } from "./landing.mjs";
import { markdown, escape } from "./markdown.mjs";

const SDK = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const REPO = path.dirname(SDK);
export const GUIDE = path.join(REPO, "docs", "guide");
export const SITE = path.join(REPO, "docs", "site");
const TYPES = path.join(SDK, "types", "alpha.d.ts");
const MASCOT = path.join(REPO, "aster.png");

// Navigation: [section, [[file (without .md), title]]]. reference is generated.
export const NAV = [
  ["Start", [["getting-started", "Getting started"], ["first-game", "Your first game"], ["concepts", "Concepts"], ["examples", "Examples"]]],
  ["Guides", [["input", "Input"], ["ui", "Game UI"], ["text", "Text and languages"], ["audio", "Audio and music"], ["physics", "Physics"],
              ["particles", "Particles, looks, layers"], ["shaders", "Shaders and lighting"], ["tilemaps", "Sprites, camera, tilemaps"],
              ["saves", "Saves, settings, i18n"], ["hot-reload", "Hot reload"], ["mcp", "MCP for agents"],
              ["testing", "Testing"], ["exporting", "Exporting"], ["android", "Android"]]],
  ["Reference", [["reference", "API reference"], ["agents", "AGENTS.md for games"]]],
];

function nav(current) {
  return NAV.map(([section, pages]) => `<h3>${section}</h3><ul>${pages.map(([file, title]) =>
    `<li><a href="${file}.html"${file === current ? ' class="here"' : ""}>${title}</a></li>`).join("")}</ul>`).join("");
}

function page(file, title, body) {
  return `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>${escape(title)} - alpha-star</title><link rel="stylesheet" href="style.css"></head>
<body><nav><a class="brand" href="index.html"><img src="aster.png" alt="">alpha-star</a>${nav(file)}</nav>
<main>${body}</main></body></html>
`;
}

// ---- API reference from alpha.d.ts ------------------------------------------------------

function jsdoc(node) {
  const docs = node.jsDoc ?? [];
  const last = docs[docs.length - 1];
  if (!last?.comment) return "";
  return typeof last.comment === "string" ? last.comment : last.comment.map((c) => c.text).join("");
}

const KIND = { [ts.SyntaxKind.InterfaceDeclaration]: "interface", [ts.SyntaxKind.TypeAliasDeclaration]: "type",
               [ts.SyntaxKind.FunctionDeclaration]: "function", [ts.SyntaxKind.VariableStatement]: "const" };

function nameOf(node) {
  if (ts.isVariableStatement(node)) return node.declarationList.declarations[0].name.getText();
  return node.name.getText();
}

// The declaration's text without its JSDoc, members' JSDoc moved into the member table.
function entry(node, sf) {
  const name = nameOf(node);
  const doc = jsdoc(node);
  const members = ts.isInterfaceDeclaration(node)
    ? node.members.map((m) => ({ sig: m.getText(sf).replace(/\s+/g, " "), doc: jsdoc(m) }))
    : [];
  const head = ts.isInterfaceDeclaration(node)
    ? node.getText(sf).slice(0, node.members.pos - node.getStart(sf)).trim().replace(/\s*\{$/, "")
    : node.getText(sf);
  return { name, kind: KIND[node.kind], doc, head, members };
}

/** { module name -> entries } read from the .d.ts with the TypeScript parser. */
export function apiEntries(source = fs.readFileSync(TYPES, "utf8")) {
  const sf = ts.createSourceFile("alpha.d.ts", source.replace(/\r\n/g, "\n"), ts.ScriptTarget.ES2022, true);
  const modules = {};
  for (const stmt of sf.statements) {
    if (!ts.isModuleDeclaration(stmt) || !stmt.body || !ts.isModuleBlock(stmt.body)) continue;
    modules[stmt.name.text] = stmt.body.statements.filter((s) => KIND[s.kind]).map((s) => entry(s, sf));
  }
  return modules;
}

function renderEntry(e) {
  const rows = e.members.map((m) => `<tr><td><code>${escape(m.sig)}</code></td><td>${markdown(m.doc, true)}</td></tr>`).join("");
  return `<section id="${e.name}"><h3><span class="kind">${e.kind}</span> <a href="#${e.name}">${e.name}</a></h3>
${e.doc ? markdown(e.doc) : ""}<pre><code>${escape(e.head)}</code></pre>
${rows ? `<table class="members"><tbody>${rows}</tbody></table>` : ""}</section>`;
}

function reference() {
  const modules = apiEntries();
  const toc = Object.entries(modules).map(([mod, list]) =>
    `<h4>"${mod}"</h4><p class="toc">${list.map((e) => `<a href="#${e.name}">${e.name}</a>`).join(" ")}</p>`).join("");
  const body = Object.entries(modules).map(([mod, list]) =>
    `<h2>module "${mod}"</h2>${list.map(renderEntry).join("\n")}`).join("\n");
  return `<h1>API reference</h1><p>Generated from <code>alpha-sdk/types/alpha.d.ts</code> (the source of truth:
the compiler checks your game against it). Every name your game declares in <code>GameTypes</code> narrows these
types, so a misspelled action, tag or asset is a compile error.</p>${toc}${body}`;
}

// ---- site --------------------------------------------------------------------------------

// The repo's mascot (aster.png) shrunk to `size` px by box-averaging (premultiplied), so the
// site does not carry the full-size image. Unreadable PNGs are copied as they are.
function icon(png, size) {
  const src = decodePng(png);
  if (!src) return png;
  const out = { width: size, height: size, rgba: Buffer.alloc(size * size * 4) };
  const sx = src.width / size, sy = src.height / size;
  for (let y = 0; y < size; y++) {
    for (let x = 0; x < size; x++) {
      const acc = [0, 0, 0, 0];
      let n = 0;
      for (let j = Math.floor(y * sy); j < Math.floor((y + 1) * sy); j++) {
        for (let i = Math.floor(x * sx); i < Math.floor((x + 1) * sx); i++, n++) {
          const o = (j * src.width + i) * 4, a = src.rgba[o + 3];
          for (let c = 0; c < 3; c++) acc[c] += src.rgba[o + c] * a;
          acc[3] += a;
        }
      }
      const o = (y * size + x) * 4;
      for (let c = 0; c < 3; c++) out.rgba[o + c] = acc[3] ? Math.round(acc[c] / acc[3]) : 0;
      out.rgba[o + 3] = Math.round(acc[3] / n);
    }
  }
  return encodePng(out);
}

/** { file name -> contents } of the whole site. */
export function buildSite() {
  const files = { "style.css": fs.readFileSync(path.join(path.dirname(fileURLToPath(import.meta.url)), "docs.css"), "utf8") };
  files["logo.png"] = fs.readFileSync(path.join(REPO, "logo.png"));
  if (fs.existsSync(MASCOT)) files["aster.png"] = icon(fs.readFileSync(MASCOT), 128);
  for (const [, pages] of NAV) {
    for (const [file, title] of pages) {
      const body = file === "reference" ? reference() : markdown(fs.readFileSync(path.join(GUIDE, `${file}.md`), "utf8"));
      files[`${file}.html`] = page(file, title, body);
    }
  }
  files["index.html"] = landing();
  for (const name of ["landing.css", "landing.js"]) {
    files[name] = fs.readFileSync(path.join(SDK, "scripts", name), "utf8");
  }
  for (const name of fs.readdirSync(path.join(GUIDE, "assets"))) {
    if (name.endsWith(".png")) files[name] = fs.readFileSync(path.join(GUIDE, "assets", name));
  }
  return files;
}

/** What differs between docs/site and a fresh build: ["reference.html differs", ...]. */
export function staleFiles() {
  const want = buildSite();
  const have = fs.existsSync(SITE) ? fs.readdirSync(SITE) : [];
  // Text compares without \r: git may check the site out with CRLF line endings.
  const same = (disk, text) => (typeof text === "string" ? disk.toString("utf8").replace(/\r\n/g, "\n") === text : disk.equals(text));
  const problems = Object.entries(want).filter(([name, text]) => {
    const file = path.join(SITE, name);
    return !fs.existsSync(file) || !same(fs.readFileSync(file), text);
  }).map(([name]) => `${name} differs`);
  return problems.concat(have.filter((n) => !(n in want)).map((n) => `${n} is not generated`));
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  if (process.argv.includes("--check")) {
    const problems = staleFiles();
    for (const p of problems) console.error(`docs/site/${p}`);
    if (problems.length) console.error("docs are stale: run `npm run docs` in alpha-sdk");
    process.exit(problems.length ? 1 : 0);
  }
  fs.rmSync(SITE, { recursive: true, force: true });
  fs.mkdirSync(SITE, { recursive: true });
  for (const [name, text] of Object.entries(buildSite())) fs.writeFileSync(path.join(SITE, name), text);
  console.log(`docs written to ${path.relative(process.cwd(), SITE)}`);
}

