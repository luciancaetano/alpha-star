// Headless tests against an in-memory server: no real network, same result every run.
import { netFake, simulate } from "alpha/test";
import game, { SERVER } from "../src/main";

export function otherPlayersShowUpAsGhosts() {
  const srv = netFake.server("ws", SERVER, { latencySteps: 3 });
  srv.onConnect((peer) => peer.send(JSON.stringify({ id: 7, x: 40, y: 50 })));
  const run = simulate(game, { seconds: 1 });
  run.expect.noErrors();
  run.expect.count("ghost").equals(1);
  const [ghost] = run.entities("ghost");
  if (ghost.pos[0] !== 40 || ghost.pos[1] !== 50) throw new Error(`ghost at ${ghost.pos}`);
}

export function movesAreSentAboutTenTimesASecond() {
  const srv = netFake.server("ws", SERVER);
  simulate(game, { seconds: 1, inputs: { axes: { moveX: [[0, 1]] } } }).expect.noErrors();
  const sent = srv.messages().map((m) => JSON.parse(m as string) as { x: number });
  if (sent.length < 8 || sent.length > 10) throw new Error(`sent ${sent.length} updates`);
  if (!(sent[sent.length - 1].x > sent[0].x)) throw new Error("x did not grow while moving right");
}

export function junkFromTheServerIsIgnored() {
  const srv = netFake.server("ws", SERVER);
  srv.onConnect((peer) => {
    peer.send("not json");
    peer.send(new Uint8Array([1, 2]));
  });
  const run = simulate(game, { seconds: 0.5 });
  run.expect.noErrors();
  run.expect.count("ghost").equals(0);
}

export function playsAloneWithNoServer() {
  const run = simulate(game, { seconds: 0.5 });
  run.expect.noErrors();
  run.expect.count("me").equals(1);
}
