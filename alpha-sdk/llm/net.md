# Networking (alpha:net)

> summary: TCP, UDP and WebSocket (ws:// and wss://) clients from `alpha:net`: opening sockets in enter, the per-step receive/send model, framing, states and errors, limits, netFake servers for tests and --net-record/--net-replay; read when a game talks to a server.

## API

```ts
import { net, type Socket } from "alpha:net";

net.tcp(host, port, { framing?: "lines" | "u16" | "u32" /*default*/ | "raw", maxMessageBytes?, connectTimeout? /*10 s*/ })
net.udp(host, port, { maxMessageBytes? /*max 1200*/ })                 // "open" at once
net.ws("ws://host:port/path" | "wss://...", { protocols?, headers?, maxMessageBytes?, connectTimeout? })

socket.kind                       // "tcp" | "udp" | "ws"
socket.state                      // "connecting" | "open" | "closing" | "closed" (closed is final)
socket.error                      // string | null: why it failed ("connect to h:7777 refused", ...)
socket.closeCode                  // WebSocket close code | null
socket.stats                      // { sent, received, queuedOut, queuedIn } bytes
socket.send(data)                 // string | Uint8Array; false = closing/closed or queue full
socket.receive()                  // [{ data: string | Uint8Array, step }]; drains
socket.close(code? /*1000*/, reason?)   // idempotent; ws codes 1000 or 3000..4999
```

## The model (no callbacks)

- At the start of each fixed step the engine delivers what arrived and the state; both stay the same during the step.
- `receive()` returns what was delivered and empties the queue (a second call in the same step returns `[]`).
- `send()` queues; bytes go out after the step. Sends while "connecting" go out once open.
- Reconnect = open a new socket. Failures never throw: the socket becomes "closed" with `error`.
- Open sockets in a scene's `enter` (or later) and keep them in a module variable; a socket opened at module top level is not seen by `simulate()` runs.

```ts
let ws: Socket;
const play = scene("play", {
  enter() { ws = net.ws("wss://game.example.com/room/1"); },
  update(ctx) {
    for (const m of ws.receive()) applyState(JSON.parse(m.data as string));
    if (ws.state === "open") ws.send(JSON.stringify({ x: ctx.find("hero")!.pos.x }));
    if (ws.state === "closed") ctx.ui.set("hud", "status", ws.error ?? "disconnected");
  },
});
```

## Data

| Transport | Message | `data` received | Order / delivery |
|---|---|---|---|
| tcp `"lines"` | UTF-8 line (`\n` removed, `\r\n` accepted) | string | guaranteed |
| tcp `"u16"` / `"u32"` | big-endian length prefix + payload | Uint8Array | guaranteed |
| tcp `"raw"` | each chunk read (no boundaries) | Uint8Array | guaranteed |
| udp | one datagram, only from host:port | Uint8Array | none (lost, duplicated, reordered) |
| ws | one WebSocket message | string (text frame) or Uint8Array (binary) | guaranteed |

`send("text")` is a text frame on ws and UTF-8 bytes on tcp/udp; `send(new Uint8Array(...))` is binary.
Ping/pong and fragmentation are handled by the engine. `wss://` verifies the server certificate
against the system's trusted roots and the host name.

## Limits

32 sockets not yet closed per game; messages up to 1 MiB (u16: 65535; UDP: 1200); 8 MiB queued
each way per socket (a full inbound queue closes the socket: call `receive()` every step); 8 KiB of
extra ws headers. Options may lower these, never raise them.

## Errors

Thrown at once (the game pauses, like any script error):
`net.tcp: port must be an integer 1..65535, got 70000`,
`net.tcp: framing must be one of lines|u16|u32|raw, got "json"`,
`net.ws: url must start with ws:// or wss://, got "http://x"`,
`net.ws: header "Host" is set by the engine`,
`net.tcp: too many open sockets (32, limit kMaxNetSockets); close unused ones`,
`socket.send: message of 70000 bytes exceeds maxMessageBytes 65536`.

In `socket.error` (state "closed"): `connect to example.com:7777 refused`,
`connect to example.com:7777 timed out after 10 s`, `dns lookup failed for "exmple.com"`,
`ws handshake failed: server answered HTTP 404`, `peer sent 2 MiB message, over maxMessageBytes 1 MiB`,
`wss: certificate of host not trusted: ...`, `inbound queue full (8 MiB): ...`.

## Tests: netFake

In `alpha test` every socket reaches an in-memory network, never the real one. Each test starts
with no servers; a socket with no server closes with `connect to ... refused (no netFake server there)`.

```ts
import { simulate, netFake } from "alpha/test";

export function joinsAndEchoes() {
  const srv = netFake.server("ws", "ws://localhost:9000/game", { latencySteps: 2 });  // tcp/udp: "host:port"
  srv.onConnect((peer) => peer.send('{"hello":1}'));
  srv.onMessage((peer, data) => peer.send(data));   // runs between simulated steps
  const run = simulate(game, { seconds: 1 });
  run.expect.noErrors();
  if (!srv.messages().length) throw new Error("the game sent nothing");
}
```

`loss` (UDP, 0..1) drops datagrams from a fixed seed: the same drops on every run. `peer.close(code)`.

## Repeating a network bug

`alpha_star <pack> --net-record net.tape` writes every delivery (per socket and step) when the
game exits; `--net-replay net.tape` delivers them again with no network (dev/release engines).
A deterministic game repeats its socket order, so the run plays out the same.

## Platforms

Android exports add the INTERNET permission only when the game's sources import `alpha:net`.
`game_state.net` (MCP `state`) lists sockets: id, kind, endpoint, state, error, stats.
