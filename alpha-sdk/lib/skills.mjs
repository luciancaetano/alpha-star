// `alpha skills`: install or update the SDK's agent skills in a game project, in the folder each
// coding agent reads. A skill is skills/<name>/<name>.md (+ scripts/, references/) in the SDK; it is
// installed as <agent dir>/<name>/SKILL.md (the Agent Skills layout every agent below reads).
import fs from "node:fs";
import path from "node:path";
import readline from "node:readline/promises";
import { fileURLToPath } from "node:url";

const SDK = path.dirname(path.dirname(fileURLToPath(import.meta.url)));

// Project skill folder per agent. `agents` (.agents/skills) is the shared convention.
export const AGENTS = [
  { id: "claude", dir: ".claude/skills", label: "Claude Code", default: true },
  { id: "agents", dir: ".agents/skills", label: "Shared .agents (Codex, Gemini CLI, Cursor, Copilot, OpenCode, Amp)", default: true },
  { id: "cursor", dir: ".cursor/skills", label: "Cursor (own folder)" },
  { id: "gemini", dir: ".gemini/skills", label: "Gemini CLI (own folder)" },
  { id: "copilot", dir: ".github/skills", label: "GitHub Copilot (own folder)" },
  { id: "opencode", dir: ".opencode/skills", label: "OpenCode (own folder)" },
];

// The SDK's skills: the packaged copy, or the repo's skills/ in a checkout.
export function skillsSource(sdk = SDK) {
  const dirs = [path.join(sdk, "skills"), path.join(sdk, "..", "skills")];
  return dirs.find((d) => fs.existsSync(d)) ?? null;
}

export function listSkills(source) {
  return fs.readdirSync(source, { withFileTypes: true })
    .filter((e) => e.isDirectory() && fs.existsSync(path.join(source, e.name, `${e.name}.md`)))
    .map((e) => e.name).sort();
}

function copyTree(from, to, rename) {
  fs.mkdirSync(to, { recursive: true });
  for (const e of fs.readdirSync(from, { withFileTypes: true })) {
    const src = path.join(from, e.name);
    if (e.isDirectory()) copyTree(src, path.join(to, e.name));
    else fs.copyFileSync(src, path.join(to, rename?.[e.name] ?? e.name));
  }
}

// Replaces <root>/<agent dir>/<skill> with the SDK's copy. Returns [{ agent, skill, action }].
export function installSkills({ root, agents, source, skills = listSkills(source) }) {
  const done = [];
  for (const id of agents) {
    const agent = AGENTS.find((a) => a.id === id);
    if (!agent) throw new Error(`unknown agent "${id}"; available: ${AGENTS.map((a) => a.id).join(", ")}`);
    for (const skill of skills) {
      const to = path.join(root, agent.dir, skill);
      const action = fs.existsSync(to) ? "updated" : "installed";
      fs.rmSync(to, { recursive: true, force: true });
      copyTree(path.join(source, skill), to, { [`${skill}.md`]: "SKILL.md" });
      done.push({ agent: id, skill, action, dir: path.join(agent.dir, skill) });
    }
  }
  return done;
}

// Interactive pick: numbers separated by commas/spaces; Enter keeps the defaults.
export async function askAgents(input = process.stdin, output = process.stdout) {
  const rl = readline.createInterface({ input, output });
  output.write("Install skills for which agents?\n");
  AGENTS.forEach((a, i) => output.write(`  ${i + 1}. [${a.default ? "x" : " "}] ${a.id.padEnd(8)} ${a.dir.padEnd(16)} ${a.label}\n`));
  const answer = await rl.question("Numbers (e.g. 1,2,4), Enter = defaults: ");
  rl.close();
  return parsePick(answer);
}

export function parsePick(answer) {
  const picks = answer.split(/[\s,]+/).filter(Boolean);
  if (!picks.length) return AGENTS.filter((a) => a.default).map((a) => a.id);
  return picks.map((p) => {
    const a = AGENTS[Number(p) - 1] ?? AGENTS.find((x) => x.id === p);
    if (!a) throw new Error(`"${p}" is not in the list (1-${AGENTS.length} or an agent id)`);
    return a.id;
  });
}
