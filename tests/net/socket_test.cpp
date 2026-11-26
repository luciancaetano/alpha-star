#include <doctest/doctest.h>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "net/fake_net.hpp"
#include "net/hub.hpp"

using namespace alpha::net;

namespace {

Address ws_at(const char* host) {
  Address a;
  a.kind = Kind::ws;
  a.host = host;
  a.port = 9000;
  a.path = "/game";
  a.max_message = 64;
  return a;
}

// One fixed step as the engine runs it: deliver, (script), flush, then the server's poll.
FakeNet::Events step(Hub& hub, FakeNet& fake, std::uint32_t server) {
  hub.deliver();
  hub.flush();
  return fake.poll(server);
}

}  // namespace

TEST_CASE("a socket connects, exchanges messages each way and closes cleanly") {
  FakeNet fake;
  const auto srv = fake.server(Kind::ws, "ws://localhost:9000/game", {});
  Hub hub(fake.factory());
  Socket& s = *hub.find(hub.open(ws_at("localhost")));
  CHECK(s.state() == State::connecting);
  CHECK(s.send({"early", true}));  // queued while connecting, sent once open
  auto ev = step(hub, fake, srv);
  CHECK(s.state() == State::open);
  REQUIRE(ev.connects.size() == 1);
  REQUIRE(ev.messages.size() == 1);
  CHECK(ev.messages[0].second.data == "early");
  CHECK(fake.send(ev.connects[0], {"{\"hello\":1}", true}));
  step(hub, fake, srv);
  const auto got = s.receive();
  REQUIRE(got.size() == 1);
  CHECK(got[0].data == "{\"hello\":1}");
  CHECK(got[0].step == hub.step());
  CHECK(s.receive().empty());  // drained
  s.close(1000, "bye");
  CHECK(s.state() == State::closing);
  CHECK_FALSE(s.send({"late", true}));
  hub.deliver();
  CHECK(s.state() == State::closed);
  CHECK(s.error().empty());
  CHECK(s.close_code() == 1000);
  CHECK(s.stats().sent == 5);
  CHECK(s.stats().received == 11);
}

TEST_CASE("a step sees a stable picture: arrivals show only at the next deliver") {
  FakeNet fake;
  const auto srv = fake.server(Kind::ws, "ws://localhost:9000/game", {});
  Hub hub(fake.factory());
  Socket& s = *hub.find(hub.open(ws_at("localhost")));
  const auto peer = step(hub, fake, srv).connects.at(0);
  fake.send(peer, {"a", true});
  hub.pump();  // a frame while paused: pumped, not visible
  CHECK(s.receive().empty());
  CHECK(s.stats().queued_in == 0);
  hub.deliver();
  CHECK(s.receive().size() == 1);
}

TEST_CASE("latency delays connect by whole steps") {
  FakeNet fake;
  const auto srv = fake.server(Kind::ws, "ws://localhost:9000/game", {2, 0});
  Hub hub(fake.factory());
  Socket& s = *hub.find(hub.open(ws_at("localhost")));
  step(hub, fake, srv);
  CHECK(s.state() == State::connecting);
  const auto ev = step(hub, fake, srv);
  CHECK(s.state() == State::open);
  CHECK(ev.connects.size() == 1);
}

TEST_CASE("no server: closed with a refusal the agent can read") {
  FakeNet fake;
  Hub hub(fake.factory());
  Socket& s = *hub.find(hub.open(ws_at("nowhere")));
  hub.deliver();
  CHECK(s.state() == State::closed);
  CHECK(s.error() == "connect to ws://nowhere:9000/game refused (no netFake server there)");
}

TEST_CASE("send refuses oversized messages; the hub caps sockets not yet closed") {
  FakeNet fake;
  Hub hub(fake.factory());
  Socket& s = *hub.find(hub.open(ws_at("x")));
  CHECK_THROWS_WITH_AS(s.send({std::string(65, 'x'), false}), "socket.send: message of 65 bytes exceeds maxMessageBytes 64",
                       alpha::core::Error);
  for (std::size_t i = 1; i < alpha::core::kMaxNetSockets; ++i) hub.open(ws_at("x"));
  CHECK_THROWS_WITH_AS(hub.open(ws_at("x")), "net.ws: too many open sockets (32, limit kMaxNetSockets); close unused ones",
                       alpha::core::Error);
  hub.deliver();  // all refused -> closed: room again
  CHECK(hub.open(ws_at("x")) > 0);
}

TEST_CASE("the server closing delivers its last messages first, then closed with its code") {
  FakeNet fake;
  const auto srv = fake.server(Kind::ws, "ws://localhost:9000/game", {});
  Hub hub(fake.factory());
  Socket& s = *hub.find(hub.open(ws_at("localhost")));
  const auto peer = step(hub, fake, srv).connects.at(0);
  fake.send(peer, {"last", true});
  fake.close(peer, 4000);
  hub.deliver();
  CHECK(s.receive().size() == 1);
  CHECK(s.state() == State::closed);
  CHECK(s.close_code() == 4000);
  CHECK(fake.poll(srv).closes.size() == 1);
}

TEST_CASE("udp is open at once and the fake drops the same datagrams every run") {
  auto run = [] {
    FakeNet fake;
    const auto srv = fake.server(Kind::udp, "h:1", {0, 0.5});
    Hub hub(fake.factory());
    Address a;
    a.kind = Kind::udp;
    a.host = "h";
    a.port = 1;
    a.max_message = 1200;
    Socket& s = *hub.find(hub.open(a));
    CHECK(s.state() == State::open);
    std::size_t got = 0;
    for (int i = 0; i < 40; ++i) {
      s.send({"d", false});
      got += step(hub, fake, srv).messages.size();
    }
    return got;
  };
  const std::size_t first = run();
  CHECK(first > 5);
  CHECK(first < 35);
  CHECK(run() == first);
}
