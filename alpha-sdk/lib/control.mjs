// Client for the engine's agent control channel ("alpha-control"): starts the
// release engine with `--control <port>` on 127.0.0.1 and talks line-delimited
// JSON to it. One request at a time, one response line per request.
// Protocol reference: src/mcp/dispatch.hpp in the alpha-star repo.
import { spawn } from "node:child_process";
import crypto from "node:crypto";
import net from "node:net";

const PROTOCOL = "alpha-control";
const CONNECT_TIMEOUT_MS = 15000;
const LOG_TAIL = 40;

/** A free TCP port on 127.0.0.1 (the OS picks; released before the engine binds it). */
export function freePort() {
  return new Promise((resolve, reject) => {
    const server = net.createServer();
    server.once("error", reject);
    server.listen(0, "127.0.0.1", () => {
      const { port } = server.address();
      server.close(() => resolve(port));
    });
  });
}

/** Splits a byte stream into lines and hands each to `onLine`. */
function lineReader(onLine) {
  let buffer = "";
  return (chunk) => {
    buffer += chunk;
    for (let nl = buffer.indexOf("\n"); nl >= 0; nl = buffer.indexOf("\n")) {
      const line = buffer.slice(0, nl).replace(/\r$/, "");
      buffer = buffer.slice(nl + 1);
      if (line) onLine(line);
    }
  };
}

export class GameClient {
  /** Starts `engine pack --control <port> [--hidden]` and connects. */
  static async start({ engine, pack, hidden = false, cwd }) {
    const port = await freePort();
    const token = crypto.randomBytes(16).toString("hex");
    const args = [pack, "--control", String(port), ...(hidden ? ["--hidden"] : [])];
    const child = spawn(engine, args, { cwd, stdio: ["ignore", "pipe", "pipe"],
                                       env: { ...process.env, ALPHA_CONTROL_TOKEN: token } });
    const client = new GameClient(child);
    await client.#connect(port, token, engine);
    return client;
  }

  #child;
  #socket = null;
  #pending = [];
  #exited = null;
  /** The engine's last stdout/stderr lines (startup problems, script errors). */
  output = [];

  constructor(child) {
    this.#child = child;
    const keep = lineReader((line) => {
      this.output.push(line);
      if (this.output.length > LOG_TAIL) this.output.shift();
    });
    child.stdout.setEncoding("utf8").on("data", keep);
    child.stderr.setEncoding("utf8").on("data", keep);
    this.exited = new Promise((resolve) => {
      child.once("error", (e) => resolve((this.#exited = `cannot start the engine: ${e.message}`)));
      child.once("exit", (code, signal) => resolve((this.#exited = `engine exited (${signal ?? `code ${code}`})`)));
    });
    this.exited.then((why) => this.#failAll(why));
  }

  get running() {
    return this.#exited === null;
  }

  async #connect(port, token, engine) {
    const deadline = Date.now() + CONNECT_TIMEOUT_MS;
    while (Date.now() < deadline && this.running) {
      try {
        this.#socket = await connectOnce(port);
        break;
      } catch {
        await new Promise((r) => setTimeout(r, 100));
      }
    }
    if (!this.#socket) {
      this.#child.kill();
      throw new Error(`no control channel from ${engine} on port ${port}` +
                      ` (${this.#exited ?? "timed out"}). Engine output:\n${this.output.join("\n")}\n` +
                      "The engine must be a release build with ALPHA_MCP (restage it with tools/stage_engine.py).");
    }
    this.#socket.setEncoding("utf8").on("data", lineReader((line) => this.#pending.shift()?.(line)));
    this.#socket.on("close", () => this.#failAll("control connection closed"));
    this.#socket.on("error", () => {});
    this.greeting = JSON.parse(await this.#nextLine());
    if (this.greeting.protocol !== PROTOCOL) throw new Error(`unexpected greeting: ${JSON.stringify(this.greeting)}`);
    await this.request("hello", { token });
  }

  #nextLine() {
    return new Promise((resolve, reject) => {
      if (!this.running || !this.#socket) return reject(new Error(this.#exited ?? "not connected"));
      this.#pending.push((line) => (line === null ? reject(new Error(this.#lastWords())) : resolve(line)));
    });
  }

  #failAll(why) {
    this.#exited ??= why;
    for (const waiter of this.#pending.splice(0)) waiter(null);
  }

  #lastWords() {
    const tail = this.output.slice(-10).join("\n");
    return `${this.#exited ?? "connection lost"}${tail ? `; engine output:\n${tail}` : ""}`;
  }

  /** Sends one command; resolves with the response object, rejects with the engine's error text. */
  async request(cmd, fields = {}) {
    const line = this.#nextLine();
    this.#socket.write(`${JSON.stringify({ cmd, ...fields })}\n`);
    const response = JSON.parse(await line);
    if (!response.ok) throw new Error(response.error);
    return response;
  }

  /** Asks the engine to quit; kills it if it does not exit within a few seconds. */
  async stop() {
    if (this.running) await this.request("quit").catch(() => {});
    const timer = setTimeout(() => this.#child.kill(), 3000);
    await this.exited;
    clearTimeout(timer);
    this.#socket?.destroy();
  }
}

function connectOnce(port) {
  return new Promise((resolve, reject) => {
    const socket = net.connect({ host: "127.0.0.1", port });
    socket.once("connect", () => resolve(socket));
    socket.once("error", reject);
  });
}
