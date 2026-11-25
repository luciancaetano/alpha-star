import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import { apiEntries, GUIDE, NAV, staleFiles } from "../scripts/make-docs.mjs";
import { markdown } from "../scripts/markdown.mjs";

const inRepo = fs.existsSync(GUIDE);

test("docs/site is what docs/guide and alpha.d.ts generate (npm run docs after changing either)", (t) => {
  if (!inRepo) return t.skip("docs sources not next to this SDK");
  assert.deepEqual(staleFiles(), []);
});

test("every guide page in the navigation exists", (t) => {
  if (!inRepo) return t.skip("docs sources not next to this SDK");
  for (const [, pages] of NAV) for (const [file] of pages) if (file !== "reference") assert.ok(fs.existsSync(`${GUIDE}/${file}.md`), file);
});

test("the reference lists every declaration of both modules with its JSDoc and members", () => {
  const api = apiEntries();
  assert.deepEqual(Object.keys(api), ["alpha", "alpha/test"]);
  const names = (m) => api[m].map((e) => e.name);
  for (const n of ["GameTypes", "Context", "Entity", "Camera", "define", "vec"]) assert.ok(names("alpha").includes(n), n);
  for (const n of ["simulate", "SimulateOptions", "Expect"]) assert.ok(names("alpha/test").includes(n), n);
  const camera = api.alpha.find((e) => e.name === "Camera");
  assert.match(camera.doc, /which part of the world/);
  const shake = camera.members.find((m) => m.sig.startsWith("shake("));
  assert.match(shake.doc, /fading out/);
});

test("a declaration added to the types shows up in the reference", () => {
  const api = apiEntries(`declare module "alpha" {\n  /** Says hi. */\n  export interface Hello {\n    /** Who. */\n    name: string;\n  }\n}\n`);
  assert.deepEqual(api.alpha, [{ name: "Hello", kind: "interface", doc: "Says hi.", head: "export interface Hello",
                                 members: [{ sig: "name: string;", doc: "Who." }] }]);
});

test("markdown: headings, fences, lists, tables, inline code, links to .md pages", () => {
  const html = markdown("# Title\n\nSome `x < y` and [guide](ui.md#fonts).\n\n- one\n- two\n\n```\na < b\n```\n\n| A | B |\n|---|---|\n| 1 | 2 |");
  assert.equal(html, [
    '<h1 id="title">Title</h1>',
    '<p>Some <code>x &lt; y</code> and <a href="ui.html#fonts">guide</a>.</p>',
    "<ul><li>one</li><li>two</li></ul>",
    "<pre><code>a &lt; b</code></pre>",
    "<table><thead><tr><th>A</th><th>B</th></tr></thead><tbody><tr><td>1</td><td>2</td></tr></tbody></table>",
  ].join("\n"));
});
