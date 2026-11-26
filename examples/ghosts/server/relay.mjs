// A tiny relay server for Ghosts: every { x, y } a player sends goes to every other
// player as { id, x, y }. Plain Node (no packages): `npm run server`, then `npm start`
// in two or more terminals. Listens on ws://localhost:9000/ghosts (PORT to change it).
import crypto from "node:crypto";
import http from "node:http";

const PORT = Number(process.env.PORT ?? 9000);
const GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const players = new Map();  // socket -> { id, buffer }
let nextId = 1;

// One server frame (unmasked, FIN set). Payloads here are small: < 64 KiB.
function frame(opcode, payload) {
  const head = payload.length < 126 ? Buffer.from([0x80 | opcode, payload.length])
    : Buffer.from([0x80 | opcode, 126, payload.length >> 8, payload.length & 255]);
  return Buffer.concat([head, payload]);
}

// Whole client frames at the start of `buf`: [{ opcode, payload }], plus the bytes left over.
function frames(buf) {
  const out = [];
  while (buf.length >= 2) {
    let len = buf[1] & 127, at = 2;
    if (len === 126) { if (buf.length < 4) break; len = buf.readUInt16BE(2); at = 4; }
    if (len === 127) { if (buf.length < 10) break; len = Number(buf.readBigUInt64BE(2)); at = 10; }
    if (buf.length < at + 4 + len) break;
    const mask = buf.subarray(at, at + 4);
    const payload = Buffer.from(buf.subarray(at + 4, at + 4 + len)).map((b, i) => b ^ mask[i % 4]);
    out.push({ opcode: buf[0] & 15, payload });
    buf = buf.subarray(at + 4 + len);
  }
  return { out, rest: buf };
}

function relay(from, text) {
  let pos;
  try { pos = JSON.parse(text); } catch { return; }
  if (typeof pos?.x !== "number" || typeof pos?.y !== "number") return;
  const msg = frame(1, Buffer.from(JSON.stringify({ id: players.get(from).id, x: pos.x, y: pos.y })));
  for (const other of players.keys()) if (other !== from) other.write(msg);
}

function onData(socket, chunk) {
  const p = players.get(socket);
  const { out, rest } = frames(Buffer.concat([p.buffer, chunk]));
  p.buffer = rest;
  for (const { opcode, payload } of out) {
    if (opcode === 1) relay(socket, payload.toString("utf8"));
    else if (opcode === 9) socket.write(frame(10, payload));            // ping -> pong
    else if (opcode === 8) { socket.end(frame(8, payload.subarray(0, 2))); }  // close: echo the code
  }
}

const server = http.createServer((_, res) => res.writeHead(426).end("WebSocket only\n"));
server.on("upgrade", (req, socket) => {
  const key = req.headers["sec-websocket-key"];
  if (req.url !== "/ghosts" || !key) return socket.end("HTTP/1.1 404 Not Found\r\n\r\n");
  const accept = crypto.createHash("sha1").update(key + GUID).digest("base64");
  socket.write(`HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ${accept}\r\n\r\n`);
  const id = nextId++;
  players.set(socket, { id, buffer: Buffer.alloc(0) });
  console.log(`player ${id} joined (${players.size} online)`);
  socket.on("data", (chunk) => onData(socket, chunk));
  socket.on("close", () => { players.delete(socket); console.log(`player ${id} left (${players.size} online)`); });
  socket.on("error", () => socket.destroy());
});
server.listen(PORT, () => console.log(`ghosts relay on ws://localhost:${PORT}/ghosts`));
