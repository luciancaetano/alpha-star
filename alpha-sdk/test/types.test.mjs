// Type-level tests of alpha.d.ts: fixtures whose `@ts-expect-error` lines must be errors
// and whose other lines must type-check (an unused directive is itself an error).
import { test } from "node:test";
import assert from "node:assert/strict";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { typecheck } from "../lib/build.mjs";

const HERE = path.dirname(fileURLToPath(import.meta.url));

test("tag data: spawn, queries, handlers, has() and variance are typed", () => {
  const problems = typecheck(path.join(HERE, "fixtures", "typed_tags", "game.ts"));
  assert.deepEqual(problems.map((p) => `${p.line}: ${p.message}`), []);
});

test("systems, events and timers: tag systems, when, emit/on payloads and timer handles are typed", () => {
  const problems = typecheck(path.join(HERE, "fixtures", "systems", "game.ts"));
  assert.deepEqual(problems.map((p) => `${p.line}: ${p.message}`), []);
});
