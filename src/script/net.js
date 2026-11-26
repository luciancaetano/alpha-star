// TCP, UDP and WebSocket clients for games: embedded module "alpha:net".
// Types: alpha-sdk/types/alpha.d.ts. Sockets never block and run no callbacks:
// at the start of each fixed step the engine delivers what arrived (receive()
// drains it) and the state; sends are queued and written after the step.
const N = globalThis.__native;

const FRAMINGS = ["lines", "u16", "u32", "raw"];
const MAX_MESSAGE = 1 << 20;  // core::kMaxNetMessageBytes
const MAX_DATAGRAM = 1200;    // core::kMaxUdpDatagramBytes
const NO_STATS = Object.freeze({ sent: 0, received: 0, queuedOut: 0, queuedIn: 0 });

function fail(message, Type = TypeError) {
  throw new Type(message);
}

function options(what, opts, known) {
  if (opts === undefined) return {};
  if (!opts || typeof opts !== "object" || Array.isArray(opts)) fail(`${what}: options must be an object like { ${known[0]}: ... }`);
  for (const k of Object.keys(opts)) {
    if (!known.includes(k)) fail(`${what}: unknown option "${k}"; options: ${known.join(", ")}`);
  }
  return opts;
}

function checkHost(what, host) {
  if (typeof host !== "string" || host === "") fail(`${what}: host must be a non-empty string like "example.com", got ${JSON.stringify(host)}`);
}

function checkPort(what, port) {
  if (!Number.isInteger(port) || port < 1 || port > 65535) fail(`${what}: port must be an integer 1..65535, got ${port}`, RangeError);
}

function maxBytes(what, n, cap) {
  if (n === undefined) return cap;
  if (!Number.isInteger(n) || n < 1 || n > cap) fail(`${what}: maxMessageBytes must be an integer 1..${cap}, got ${n}`, RangeError);
  return n;
}

function timeout(what, t) {
  if (t === undefined) return 10;
  if (typeof t !== "number" || !(t > 0 && t <= 120)) fail(`${what}: connectTimeout must be seconds in (0, 120], got ${t}`, RangeError);
  return t;
}

const TOKEN = /^[!#$%&'*+.^_`|~0-9A-Za-z-]+$/;

function wsExtras(protocols = [], headers = {}) {
  if (!Array.isArray(protocols) || !protocols.every((p) => typeof p === "string" && TOKEN.test(p))) {
    fail(`net.ws: protocols must be an array of tokens like ["chat"], got ${JSON.stringify(protocols)}`);
  }
  if (!headers || typeof headers !== "object" || Array.isArray(headers)) fail("net.ws: headers must be an object like { Authorization: \"Bearer ...\" }");
  const flat = [];
  for (const [name, value] of Object.entries(headers)) {
    if (!TOKEN.test(name)) fail(`net.ws: header name ${JSON.stringify(name)} is not a valid HTTP header name`);
    if (typeof value !== "string" || /[\r\n]/.test(value)) fail(`net.ws: header ${name} must be a string without line breaks, got ${JSON.stringify(value)}`);
    flat.push(name, value);
  }
  return { protocols, headers: flat };
}

function utf8Length(s) {
  let n = 0;
  for (const ch of s) {
    const c = ch.codePointAt(0);
    n += c < 0x80 ? 1 : c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
  }
  return n;
}

function checkClose(kind, code, reason) {
  if (kind === "ws" && !(code === 1000 || (Number.isInteger(code) && code >= 3000 && code <= 4999))) {
    fail(`socket.close: code must be 1000 or 3000..4999, got ${code}`, RangeError);
  }
  if (typeof reason !== "string" || utf8Length(reason) > 123) fail("socket.close: reason must be a string of at most 123 UTF-8 bytes", RangeError);
}

function socket(kind, id) {
  let last = { state: "closed", error: null, closeCode: null, stats: NO_STATS };
  const info = () => (last = N.netInfo(id) ?? last);
  return Object.freeze({
    kind,
    get state() { return info().state; },
    get error() { return info().error; },
    get closeCode() { return info().closeCode; },
    get stats() { return info().stats; },
    send(data) {
      if (typeof data !== "string" && !(data instanceof Uint8Array)) fail(`socket.send: data must be a string or a Uint8Array, got ${data === null ? "null" : typeof data}`);
      return N.netInfo(id) ? N.netSend(id, data) : false;  // released: closed long ago
    },
    receive: () => (N.netInfo(id) ? N.netReceive(id) : []),
    close(code = 1000, reason = "") {
      checkClose(kind, code, reason);
      if (N.netInfo(id)) N.netClose(id, code, reason);
    },
  });
}

function open(kind, host, port, opts) {
  return socket(kind, N.netOpen(kind, host, port, opts));
}

export const net = Object.freeze({
  tcp(host, port, opts) {
    const o = options("net.tcp", opts, ["framing", "maxMessageBytes", "connectTimeout"]);
    checkHost("net.tcp", host);
    checkPort("net.tcp", port);
    const framing = o.framing ?? "u32";
    if (!FRAMINGS.includes(framing)) fail(`net.tcp: framing must be one of ${FRAMINGS.join("|")}, got ${JSON.stringify(framing)}`);
    const cap = framing === "u16" ? 65535 : MAX_MESSAGE;
    return open("tcp", host, port, { framing, maxMessageBytes: maxBytes("net.tcp", o.maxMessageBytes, cap),
      connectTimeout: timeout("net.tcp", o.connectTimeout), protocols: [], headers: [] });
  },
  udp(host, port, opts) {
    const o = options("net.udp", opts, ["maxMessageBytes"]);
    checkHost("net.udp", host);
    checkPort("net.udp", port);
    return open("udp", host, port, { framing: "raw", maxMessageBytes: maxBytes("net.udp", o.maxMessageBytes, MAX_DATAGRAM),
      connectTimeout: 10, protocols: [], headers: [] });
  },
  ws(url, opts) {
    const o = options("net.ws", opts, ["protocols", "headers", "maxMessageBytes", "connectTimeout"]);
    if (typeof url !== "string") fail(`net.ws: url must be a string like "ws://localhost:9000/game", got ${JSON.stringify(url)}`);
    return open("ws", url, 0, { framing: "raw", maxMessageBytes: maxBytes("net.ws", o.maxMessageBytes, MAX_MESSAGE),
      connectTimeout: timeout("net.ws", o.connectTimeout), ...wsExtras(o.protocols, o.headers) });
  },
});
