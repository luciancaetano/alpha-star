// Ghosts: move your square; every other player on the server shows up as a ghost.
// The game sends { x, y } over a WebSocket a few times a second and draws whatever
// { id, x, y } the server relays. With no server it still plays, alone.
import { define, vec } from "alpha";
import { net, type Socket } from "alpha:net";

export interface Ghosts {
  actions: never;
  axes: "moveX" | "moveY";
  sounds: never;
  images: never;
  tags: { me: {}; ghost: { peer: number } };
  ui: {};
  scenes: { play: undefined };
}

export const SERVER = "ws://localhost:9000/ghosts";
const VIEW = { width: 320, height: 180 };
const SPEED = 90;          // px/s
const SEND_EVERY = 6;      // steps between position updates (10 per second)

const { scene, game } = define<Ghosts>();
let ws: Socket;
let step = 0;

interface Seen { id: number; x: number; y: number }

function seen(data: string | Uint8Array): Seen | null {
  if (typeof data !== "string") return null;
  try {
    const m = JSON.parse(data) as Seen;
    return Number.isInteger(m.id) && typeof m.x === "number" && typeof m.y === "number" ? m : null;
  } catch {
    return null;  // a server sending junk must not stop the game
  }
}

const play = scene("play", {
  enter(ctx) {
    ctx.spawn({ tags: ["me"], pos: [VIEW.width / 2, VIEW.height / 2], shape: { rect: [10, 10], color: "#ffcc00" } });
    ws = net.ws(SERVER);
    step = 0;
  },
  update(ctx) {
    const me = ctx.find("me")!;
    me.pos = vec.add([me.pos.x, me.pos.y], vec.scale([ctx.input.axis("moveX"), ctx.input.axis("moveY")], SPEED * ctx.dt));
    for (const m of ws.receive()) {
      const s = seen(m.data);
      if (!s) continue;
      const ghost = ctx.findAll("ghost").find((g) => g.data.peer === s.id);
      if (ghost) ghost.pos = [s.x, s.y];
      else ctx.spawn({ tags: ["ghost"], data: { peer: s.id }, pos: [s.x, s.y], shape: { rect: [10, 10], color: "#8ecae6", alpha: 0.5 } });
    }
    if (ws.state === "open" && ++step % SEND_EVERY === 0) ws.send(JSON.stringify({ x: Math.round(me.pos.x), y: Math.round(me.pos.y) }));
  },
});

export default game({
  config: {
    title: "Ghosts",
    size: [VIEW.width, VIEW.height],
    background: "#1d2b53",
    seed: 1,
    actions: {},
    axes: { moveX: ["GamepadLeftX", "A/D", "Left/Right"], moveY: ["GamepadLeftY", "W/S", "Up/Down"] },
  },
  assets: { sounds: {}, images: {} },
  start: "play",
  scenes: { play },
});
