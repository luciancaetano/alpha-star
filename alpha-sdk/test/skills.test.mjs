import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { installSkills, listSkills, parsePick, skillsSource } from "../lib/skills.mjs";

function tmp() {
  return fs.mkdtempSync(path.join(os.tmpdir(), "alpha-skills-"));
}

test("skills: the repo's alpha-harness is found", () => {
  assert.ok(listSkills(skillsSource()).includes("alpha-harness"));
});

test("skills: installs as <agent dir>/<name>/SKILL.md with its files, then updates in place", () => {
  const root = tmp();
  const first = installSkills({ root, agents: ["claude", "agents"], source: skillsSource(), skills: ["alpha-harness"] });
  assert.deepEqual(first.map((d) => d.action), ["installed", "installed"]);
  for (const dir of [".claude/skills", ".agents/skills"]) {
    assert.match(fs.readFileSync(path.join(root, dir, "alpha-harness", "SKILL.md"), "utf8"), /^---\nname: alpha-harness/);
    assert.ok(fs.existsSync(path.join(root, dir, "alpha-harness", "scripts", "harness.mjs")));
  }
  fs.writeFileSync(path.join(root, ".claude/skills/alpha-harness/stale.txt"), "old");
  const again = installSkills({ root, agents: ["claude"], source: skillsSource(), skills: ["alpha-harness"] });
  assert.equal(again[0].action, "updated");
  assert.ok(!fs.existsSync(path.join(root, ".claude/skills/alpha-harness/stale.txt")));
});

test("skills: picks by number or id; Enter = claude + agents; unknown names the list", () => {
  assert.deepEqual(parsePick(""), ["claude", "agents"]);
  assert.deepEqual(parsePick("1, 3 opencode"), ["claude", "cursor", "opencode"]);
  assert.throws(() => parsePick("9"), /not in the list \(1-6/);
  assert.throws(() => installSkills({ root: tmp(), agents: ["vim"], source: skillsSource() }), /unknown agent "vim"; available: claude/);
});
