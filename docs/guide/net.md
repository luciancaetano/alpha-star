# Networking

**What you'll build:** a game that joins a WebSocket server, shows what the server says and
answers it, and a test that plays it against a fake server with no network.

`alpha:net` gives a game TCP, UDP and WebSocket (`ws://` and `wss://`) clients. There are no
callbacks: at the start of each fixed step the engine delivers what arrived, and during the step
the socket's state and messages stay the same. You read with `receive()` and write with `send()`.

## 1. Open a socket in `enter`

```ts
import { define } from "alpha";
import { net, type Socket } from "alpha:net";

interface Chat {
  actions: "say";
  axes: never;
  sounds: never;
  images: never;
  tags: {};
  ui: { hud: "status" | "last" };
  scenes: "play";
}

const { scene, game } = define<Chat>();
let ws: Socket;

const play = scene("play", {
  enter() {
    ws = net.ws("ws://localhost:9000/chat");
  },
  update(ctx) {
    for (const m of ws.receive()) ctx.ui.set("hud", "last", String(m.data));
    if (ctx.input.pressed("say")) ws.send("hello");
    ctx.ui.set("hud", "status", ws.state === "closed" ? `offline: ${ws.error ?? "closed"}` : ws.state);
  },
});

export default game({
  config: { title: "Chat", size: [320, 180], actions: { say: ["Space"] } },
  assets: { ui: { hud: "ui/hud.rml" } },
  start: "play",
  scenes: { play },
});
```

Open sockets in `enter` (or later), not at module top level: test runs (`simulate`) only see
sockets their own game instance opened. A socket that fails never throws; it becomes `"closed"`
and `ws.error` says why (`connect to localhost:9000 refused`, `ws handshake failed: server
answered HTTP 404`, ...). To reconnect, open a new socket.

## 2. Pick a transport

| | `net.tcp(host, port, opts)` | `net.udp(host, port, opts)` | `net.ws(url, opts)` |
|---|---|---|---|
| Message | framed by `framing` | one datagram | one WebSocket message |
| Order / delivery | guaranteed | none | guaranteed |
| `data` | string for `"lines"`, else `Uint8Array` | `Uint8Array` | string (text) or `Uint8Array` (binary) |

TCP framing: `"u32"` (default) and `"u16"` put the payload length first (big-endian); `"lines"`
sends UTF-8 lines ending in `\n`; `"raw"` delivers each chunk read, with no boundaries.
WebSocket: `protocols` and `headers` (for example `Authorization`) go in the handshake; ping/pong
and fragments are handled for you. `wss://` checks the server's certificate against the
system's trusted roots.

Limits: 32 open sockets, 1 MiB per message (UDP 1200 bytes), 8 MiB queued each way per socket.
Call `receive()` every step: a socket whose inbound queue fills up closes with an error.

## 3. Test it against a fake server

In `alpha test`, sockets reach an in-memory network instead of the real one:

```ts
import { simulate, netFake } from "alpha/test";
import game from "../src/main";

export function showsWhatTheServerSays() {
  const srv = netFake.server("ws", "ws://localhost:9000/chat", { latencySteps: 3 });
  srv.onConnect((peer) => peer.send("welcome"));
  const run = simulate(game, { seconds: 0.5, inputs: { say: [20] } });
  run.expect.noErrors();
  run.expect.text("hud", "last").equals("welcome");
  if (srv.messages()[0] !== "hello") throw new Error(`server got ${String(srv.messages()[0])}`);
}
```

Each test starts with no servers. Latency is in steps and UDP `loss` uses a fixed seed, so a test
passes or fails the same way every time.

## 4. Repeat a network bug

Run the engine with `--net-record net.tape` to write everything the game's sockets received,
step by step, when it exits; run it again with `--net-replay net.tape` to deliver the same
messages with no network. MCP agents see the sockets in `state` (`net`: kind, endpoint, state,
error, byte counts).

Android exports ask for the INTERNET permission only when the game's code imports `alpha:net`.
