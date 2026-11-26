#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "host_fixture.hpp"
#include "platform/vfs.hpp"

using alpha::script::Host;
using alpha::script::TestResult;
using alpha::test::Sandbox;

namespace {

// A game that opens a WebSocket on enter, keeps what it receives in state and
// answers each message with "ack:<message>".
constexpr const char* kGame = R"(
import { define } from 'alpha'; import { net } from 'alpha:net';
const { scene, game } = define();
let ws;
const play = scene('play', {
  state: () => ({ got: [], state: '', error: null }),
  enter() { ws = net.ws('ws://localhost:9000/game'); },
  update(ctx) {
    for (const m of ws.receive()) { ctx.state.got.push(m.data); ws.send('ack:' + m.data); }
    ctx.state.state = ws.state;
    ctx.state.error = ws.error;
  },
});
export default game({ config: { title: 't', size: [100, 100] }, start: 'play', scenes: { play } });
)";

std::vector<TestResult> run(const Sandbox& box, const std::string& tests) {
  const std::string game = std::filesystem::path(box.write("game.js", kGame)).generic_string();
  Host host(box.save());
  return host.test(box.write("tests.js", "import { simulate, netFake } from 'alpha/test';\nimport { net } from 'alpha:net';\n"
                                         "import game from '" + game + "';\n" + tests));
}

std::string only_error(const Sandbox& box, const std::string& tests) {
  const auto results = run(box, tests);
  REQUIRE(results.size() == 1);
  return results[0].error;
}

}  // namespace

TEST_CASE("a game talks to a netFake server: connect, messages both ways, step order") {
  Sandbox box;
  CHECK(only_error(box, R"(
export function echo() {
  const srv = netFake.server('ws', 'ws://localhost:9000/game', { latencySteps: 2 });
  srv.onConnect((peer) => peer.send('hello'));
  const r = simulate(game, { seconds: 0.5 });
  r.expect.noErrors();
  if (r.state.state !== 'open') throw new Error('state ' + r.state.state);
  if (JSON.stringify(r.state.got) !== '["hello"]') throw new Error('got ' + JSON.stringify(r.state.got));
  if (JSON.stringify(srv.messages()) !== '["ack:hello"]') throw new Error('server saw ' + JSON.stringify(srv.messages()));
}
)") == "");
}

TEST_CASE("each test starts with a fresh fake network; no server means a readable refusal") {
  Sandbox box;
  const auto results = run(box, R"(
export function a_server() { netFake.server('ws', 'ws://localhost:9000/game'); simulate(game, { seconds: 0.1 }).expect.noErrors(); }
export function b_none() {
  const r = simulate(game, { seconds: 0.1 });
  r.expect.noErrors();
  if (r.state.state !== 'closed') throw new Error('state ' + r.state.state);
  if (!r.state.error.includes('refused')) throw new Error('error ' + r.state.error);
}
)");
  REQUIRE(results.size() == 2);
  CHECK(results[0].error == "");
  CHECK(results[1].error == "");
}

TEST_CASE("boundary errors name what failed and what is allowed") {
  Sandbox box;
  const std::string e1 = only_error(box, "export function t() { net.tcp('example.com', 70000); }");
  CHECK(e1.find("net.tcp: port must be an integer 1..65535, got 70000") != std::string::npos);
  const std::string e2 = only_error(box, "export function t() { net.tcp('h', 1, { framing: 'json' }); }");
  CHECK(e2.find("net.tcp: framing must be one of lines|u16|u32|raw, got \"json\"") != std::string::npos);
  const std::string e3 = only_error(box, "export function t() { net.ws('http://x'); }");
  CHECK(e3.find("net.ws: url must start with ws:// or wss://, got \"http://x\"") != std::string::npos);
  const std::string e4 = only_error(box, "export function t() { net.ws('ws://h/', { headers: { Host: 'x' } }); }");
  CHECK(e4.find("header \"Host\" is set by the engine") != std::string::npos);
  const std::string e5 = only_error(box, "export function t() { net.ws('ws://h/', { maxMessageBytes: 10 }).send('x'.repeat(11)); }");
  CHECK(e5.find("socket.send: message of 11 bytes exceeds maxMessageBytes 10") != std::string::npos);
  const std::string e6 = only_error(box, "export function t() { net.udp('h', 1, { framing: 'lines' }); }");
  CHECK(e6.find("net.udp: unknown option \"framing\"; options: maxMessageBytes") != std::string::npos);
}

TEST_CASE("tcp lines are strings, udp datagrams are bytes; close is idempotent") {
  Sandbox box;
  CHECK(only_error(box, R"(
import { define } from 'alpha';
export function kinds() {
  const t = netFake.server('tcp', 'h:7', {});
  const u = netFake.server('udp', 'h:8', {});
  t.onConnect((p) => p.send('line'));
  u.onMessage((p) => p.send(new Uint8Array([1, 2, 3])));
  let tcp, udp;
  const out = { lines: [], bytes: [], after: [] };
  const { scene, game: make } = define();
  const play = scene('play', {
    enter() { tcp = net.tcp('h', 7, { framing: 'lines' }); udp = net.udp('h', 8); udp.send('ping'); },
    update() {
      for (const m of tcp.receive()) {
        out.lines.push(m.data);
        tcp.close(); tcp.close();
        out.after.push(tcp.state, tcp.send('x'));
      }
      for (const m of udp.receive()) out.bytes.push(m.data instanceof Uint8Array ? [...m.data] : 'not bytes');
    },
  });
  const g = make({ config: { title: 't', size: [100, 100] }, start: 'play', scenes: { play } });
  simulate(g, { seconds: 0.2 }).expect.noErrors();
  const want = '{"lines":["line"],"bytes":[[1,2,3]],"after":["closing",false]}';
  if (JSON.stringify(out) !== want) throw new Error(JSON.stringify(out));
  if (JSON.stringify(u.messages().map((d) => [...d])) !== '[[112,105,110,103]]') throw new Error('udp sent ' + JSON.stringify(u.messages()));
}
)") == "");
}

TEST_CASE("--net-record writes a tape when the host goes; a bad --net-replay file is logged") {
  Sandbox box;
  const std::string tape = (box.dir / "net.tape").string();
  {
    Host host(box.save());
    host.tape_net(tape, "");
  }
  CHECK(alpha::platform::vfs::read_text(tape).value_or("").starts_with("alpha-net-tape 1"));
  Host replay(box.save());
  replay.tape_net("", box.write("bad.tape", "not a tape"));
  bool logged = false;
  for (std::size_t i = 0; i < replay.log().size(); ++i) logged |= replay.log().at(i).text.find("--net-replay") == 0;
  CHECK(logged);
  Host good(box.save());
  good.tape_net("", tape);
  CHECK(good.log().size() == 0);
}
