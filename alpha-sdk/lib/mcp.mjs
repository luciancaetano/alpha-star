// `alpha mcp`: an MCP server over stdio that lets an AI agent build, test,
// run, drive and observe an alpha-star game. Run it from the game project
// folder. One game at a time; the game is stopped when the server exits.
import path from "node:path";
import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { z } from "zod";
import { formatProblem, typecheck } from "./build.mjs";
import { GameClient } from "./control.mjs";
import { hostTarget } from "./engine.mjs";
import { exportPack } from "./pack.mjs";
import { runTests } from "./test.mjs";

const text = (t) => ({ content: [{ type: "text", text: t }] });
const failure = (t) => ({ ...text(t), isError: true });
const json = (value) => text(JSON.stringify(value, null, 2));
const problemsText = (problems) => problems.map(formatProblem).join("\n");

const player = z.number().int().min(1).max(4).optional().describe("Player 1..4 (default 1).");
const entry = z.string().optional().describe("Game entry file, relative to the project (default src/main.ts).");

/** Every tool, as [name, description, input shape, handler(args, ctx)]. */
function tools() {
  return [
    ["alpha_check", "Type-check the game (TypeScript against the alpha-star API). Use after every code change; " +
      "fast, writes nothing. Returns 'no problems' or one 'file:line:col message' per problem.",
     { entry }, async (a, c) => {
       const problems = typecheck(c.file(a.entry, "src/main.ts"));
       return problems.length ? failure(problemsText(problems)) : text("no problems");
     }],
    ["alpha_export", "Type-check, bundle and copy assets into a pack directory the engine runs (like `alpha export`). " +
      "standalone=true also copies the player engine in. Returns the pack path and file count, or the problems.",
     { entry, out: z.string().optional().describe("Pack directory (default dist)."),
       standalone: z.boolean().optional().describe("Copy the shipping engine in, for players.") },
     async (a, c) => {
       const out = c.file(a.out, "dist");
       const r = await exportPack({ root: c.root, entry: c.file(a.entry, "src/main.ts"), out,
                                    target: a.standalone ? hostTarget() : null });
       return r.ok ? text(`exported ${out} (main.js + ${r.files.length} asset file(s))`) : failure(problemsText(r.problems));
     }],
    ["alpha_test", "Run the game's tests headlessly (test/main.test.ts by default; simulate() + expect). " +
      "Returns one PASS/FAIL line per test with the failure reason; isError when any test fails.",
     { entry: z.string().optional().describe("Test entry file (default test/main.test.ts).") }, async (a, c) => {
       const r = await runTests({ root: c.root, entry: c.file(a.entry, "test/main.test.ts"), engine: c.engine });
       const report = r.problems.length ? problemsText(r.problems) : r.output;
       return r.ok ? text(report) : failure(report);
     }],
    ["game_start", "Export the game and launch it in the engine with the agent control channel, replacing any game " +
      "this server started before. Windowed by default (the user sees it); hidden=true runs without a window. " +
      "The game runs in real time until game_pause. Returns the first game_state.",
     { entry, out: z.string().optional().describe("Pack directory (default dist)."),
       hidden: z.boolean().optional().describe("No visible window (default false).") },
     async (a, c) => c.start(a)],
    ["game_reload", "Hot reload: re-export the game started by game_start (same pack) and reload it in the running " +
      "engine, without restarting it. Keeps the save, the current scene and its enter params, and ctx.state when it " +
      "is plain JSON and the scene still declares state (else state(params) runs again); entities are respawned by " +
      "the scene's enter. Type errors or a game that fails to load change nothing (the old game keeps running) and " +
      "are returned as the error. Returns the new game_state.",
     { entry }, async (a, c) => c.reload(a)],
    ["game_state", "Snapshot of the running game: scene, paused, error (the script error that paused the game, " +
      "mapped to your TypeScript source, or null), frame (fixed steps so far), fps, entities " +
      "[{id, tags, pos, rotation, z, velocity}] and ui {document: {shown, texts{elementId: text}}}.",
     {}, async (a, c) => json(await c.state())],
    ["game_step", "Advance exactly N fixed steps (1/60 s each), then return the state. Pauses the game first if it " +
      "is running, so results are deterministic. Use after game_input to see what an input does.",
     { frames: z.number().int().min(1).max(36000).optional().describe("Steps to run (default 1).") },
     async (a, c) => {
       await c.game().request("pause");
       await c.game().request("step", { frames: a.frames ?? 1 });
       return json(await c.state());
     }],
    ["game_pause", "Pause the game (no steps run until game_step or game_resume). Script errors pause it too.",
     {}, async (a, c) => (await c.game().request("pause"), text("paused"))],
    ["game_resume", "Resume real-time play (60 steps per second) after game_pause or game_step.",
     {}, async (a, c) => (await c.game().request("resume"), text("running"))],
    ["game_input", "Inject input like a real device. press = the action is down for exactly one step; hold = down " +
      "until release; release = let go; axis = hold an analog axis at value -1..1 (omit value to release it). " +
      "Names come from the game's config.actions / config.axes; an unknown name lists the declared ones.",
     { kind: z.enum(["press", "hold", "release", "axis"]),
       action: z.string().optional().describe("Action name (press/hold/release)."),
       axis: z.string().optional().describe("Axis name (kind axis)."),
       value: z.number().min(-1).max(1).optional().describe("Axis value; omit to release the axis."),
       player },
     async (a, c) => {
       const who = { player: a.player ?? 1 };
       if (a.kind === "axis") await c.game().request("axis", { axis: a.axis, value: a.value ?? null, ...who });
       else await c.game().request(a.kind, { action: a.action, ...who });
       return text(`${a.kind} ${a.action ?? a.axis} (player ${who.player}) sent; it takes effect on the next step`);
     }],
    ["game_click", "Click a UI element as a player would (mouse, Enter or gamepad A). target = \"document#elementId\" " +
      "(see game_state.ui). The element's click handler runs on the next step. Fails if the document is hidden.",
     { target: z.string().describe("\"document#elementId\", e.g. \"over#again\".") },
     async (a, c) => (await c.game().request("click", { target: a.target }), text(`clicked ${a.target}`))],
    ["game_screenshot", "PNG image of the current frame (game + UI, at window size). Use to check visuals " +
      "(layout, colors, what the player sees).",
     {}, async (a, c) => {
       const r = await c.game().request("screenshot");
       return { content: [{ type: "image", data: r.png, mimeType: "image/png" },
                          { type: "text", text: `${r.width}x${r.height}` }] };
     }],
    ["game_logs", "console.log/warn/error lines and script errors since a cursor. Pass the returned `next` as " +
      "`since` next time to get only new lines; `dropped` counts lines too old to keep.",
     { since: z.number().int().min(0).optional().describe("Cursor from the previous call (default 0).") },
     async (a, c) => {
       const r = await c.game().request("logs", { since: a.since ?? 0 });
       return json({ lines: r.lines, next: r.next, dropped: r.dropped });
     }],
    ["game_stop", "Close the game started by game_start (the engine quits cleanly). Also happens when this server exits.", {}, async (a, c) => c.stop()],
  ];
}

/** Server state: the project, the engine, and at most one running game. */
class Context {
  constructor({ root, engine }) {
    this.root = root;
    this.engine = engine;
    this.client = null;
  }

  file(relative, fallback) {
    return path.resolve(this.root, relative ?? fallback);
  }

  game() {
    if (!this.client?.running) throw new Error("no game is running: call game_start first");
    return this.client;
  }

  async state() {
    const { id, ok, ...state } = await this.game().request("state");
    return state;
  }

  async start(a) {
    await this.client?.stop();
    this.client = null;
    const out = this.file(a.out, "dist");
    const r = await exportPack({ root: this.root, entry: this.file(a.entry, "src/main.ts"), out, target: null });
    if (!r.ok) return failure(`export failed:\n${problemsText(r.problems)}`);
    this.client = await GameClient.start({ engine: this.engine, pack: out, hidden: a.hidden ?? false, cwd: this.root });
    this.pack = out;
    return json(await this.state());
  }

  async reload(a) {
    const game = this.game();
    const r = await exportPack({ root: this.root, entry: this.file(a.entry, "src/main.ts"), out: this.pack, target: null });
    if (!r.ok) return failure(`export failed; the running game was not changed:\n${problemsText(r.problems)}`);
    await game.request("reload");
    return json(await this.state());
  }

  async stop() {
    if (!this.client) return text("no game was running");
    await this.client.stop();
    this.client = null;
    return text("stopped");
  }
}

/** The MCP server with every alpha-star tool registered (not yet connected). */
export function createServer({ root, engine, version }) {
  const server = new McpServer({ name: "alpha-star", version });
  const context = new Context({ root, engine });
  for (const [name, description, inputSchema, handler] of tools()) {
    server.registerTool(name, { description, inputSchema }, async (args) => {
      try {
        return await handler(args ?? {}, context);
      } catch (e) {
        return failure(e.message);
      }
    });
  }
  return { server, context };
}

/** Serves MCP on stdin/stdout until stdin closes; stops the game on the way out. */
export async function runMcp({ root, engine, version }) {
  const { server, context } = createServer({ root, engine, version });
  const closed = new Promise((resolve) => {
    process.stdin.once("end", resolve);
    process.once("SIGINT", resolve);
    process.once("SIGTERM", resolve);
  });
  await server.connect(new StdioServerTransport());
  await closed;
  await context.stop().catch(() => {});
  await server.close();
  return 0;
}
