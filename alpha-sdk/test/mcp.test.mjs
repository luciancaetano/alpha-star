import { test } from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { StdioClientTransport } from "@modelcontextprotocol/sdk/client/stdio.js";
import { resolveEngine } from "../lib/engine.mjs";

const SDK = path.dirname(path.dirname(fileURLToPath(import.meta.url)));
const FLAPPY = path.join(SDK, "..", "examples", "flappy");
const ENGINE = resolveEngine("");
const ENGINE_MISSING = !fs.existsSync(ENGINE)
  ? `no engine staged at ${ENGINE}: run python tools/stage_engine.py (or set ALPHA_STAR) to run the MCP end-to-end test`
  : false;

const TOOLS = ["alpha_check", "alpha_export", "alpha_test", "game_click", "game_input", "game_logs", "game_pause",
               "game_reload", "game_resume", "game_screenshot", "game_start", "game_state", "game_step", "game_stop"];

/** `alpha mcp` started from `cwd`, connected as an MCP client. */
async function connect(cwd) {
  const transport = new StdioClientTransport({ command: process.execPath, args: [path.join(SDK, "bin", "alpha.mjs"), "mcp"],
                                               cwd, env: { ...process.env }, stderr: "pipe" });
  const client = new Client({ name: "alpha-sdk-test", version: "0.0.0" });
  await client.connect(transport);
  return client;
}

const call = (client, name, args = {}) => client.callTool({ name, arguments: args });
const textOf = (result) => result.content.find((c) => c.type === "text")?.text ?? "";
const stateOf = (result) => {
  assert.ok(!result.isError, textOf(result));
  return JSON.parse(textOf(result));
};

test("alpha mcp lists every tool, each with a description for agents", async () => {
  const client = await connect(FLAPPY);
  try {
    const { tools } = await client.listTools();
    assert.deepEqual(tools.map((t) => t.name).sort(), TOOLS);
    for (const t of tools) assert.ok(t.description.length > 30, `${t.name} needs a real description`);
    const noGame = await call(client, "game_state");
    assert.ok(noGame.isError);
    assert.match(textOf(noGame), /game_start/);
  } finally {
    await client.close();
  }
});

test("alpha mcp drives Flappy end to end with the real engine", { skip: ENGINE_MISSING, timeout: 120000 }, async () => {
  const out = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-mcp-"));
  const client = await connect(FLAPPY);
  try {
    assert.equal(textOf(await call(client, "alpha_check")), "no problems");
    const started = stateOf(await call(client, "game_start", { hidden: true, out }));
    assert.equal(started.scene, "title");
    assert.equal(started.error, null);
    assert.ok(started.entities.some((e) => e.tags.includes("bird")));
    await call(client, "game_pause");
    const input = await call(client, "game_input", { kind: "press", action: "flap" });
    assert.ok(!input.isError, textOf(input));
    const played = stateOf(await call(client, "game_step", { frames: 2 }));
    assert.equal(played.scene, "play");
    assert.equal(played.paused, true);
    assert.ok(played.ui.hud.shown);
    const bad = await call(client, "game_input", { kind: "press", action: "jump" });
    assert.ok(bad.isError);
    assert.match(textOf(bad), /flap/);
    const shot = await call(client, "game_screenshot");
    const image = shot.content.find((c) => c.type === "image");
    assert.equal(image.mimeType, "image/png");
    assert.equal(Buffer.from(image.data, "base64").subarray(1, 4).toString(), "PNG");
    const logs = JSON.parse(textOf(await call(client, "game_logs")));
    assert.ok(Array.isArray(logs.lines));
    assert.equal(textOf(await call(client, "game_stop")), "stopped");
    assert.ok((await call(client, "game_state")).isError);
  } finally {
    await client.close();
    fs.rmSync(out, { recursive: true, force: true });
  }
});

/** A copy of Flappy (sources, assets, tests) to edit freely. */
function flappyCopy() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "alpha-flappy-"));
  for (const item of ["src", "assets", "test", "package.json", "tsconfig.json"]) {
    fs.cpSync(path.join(FLAPPY, item), path.join(root, item), { recursive: true });
  }
  return root;
}

test("game_reload picks up a code change in the running game; a type error changes nothing",
     { skip: ENGINE_MISSING, timeout: 120000 }, async () => {
  const root = flappyCopy();
  const tuning = path.join(root, "src", "tuning.ts");
  const client = await connect(root);
  // The title bird bobs around ART.titleY (+-6 px): moving titleY by 100 moves it for sure.
  const birdY = (state) => state.entities.find((e) => e.tags.includes("bird")).pos[1];
  const near = (y, at) => assert.ok(Math.abs(y - at) <= 6, `bird y ${y} not within 6 px of ${at}`);
  try {
    const started = stateOf(await call(client, "game_start", { hidden: true }));
    near(birdY(started), 244);
    fs.writeFileSync(tuning, fs.readFileSync(tuning, "utf8").replace("titleY: 244", "titleY: 144"));
    const reloaded = stateOf(await call(client, "game_reload"));
    assert.equal(reloaded.scene, "title");
    assert.equal(reloaded.error, null);
    near(birdY(reloaded), 144);
    fs.writeFileSync(tuning, fs.readFileSync(tuning, "utf8").replace("titleY: 144", "titleY: \"high\""));
    const bad = await call(client, "game_reload");
    assert.ok(bad.isError);
    assert.match(textOf(bad), /not changed[\s\S]*title\.ts:\d+:\d+: error/);
    near(birdY(stateOf(await call(client, "game_state"))), 144);
    await call(client, "game_stop");
  } finally {
    await client.close();
    fs.rmSync(root, { recursive: true, force: true });
  }
});
