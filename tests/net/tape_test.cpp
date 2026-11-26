#include <doctest/doctest.h>

#include "core/error.hpp"
#include "net/fake_net.hpp"
#include "net/hub.hpp"
#include "net/tape.hpp"

using namespace alpha::net;

namespace {

Address ws_here() {
  Address a;
  a.kind = Kind::ws;
  a.host = "h";
  a.port = 1;
  a.max_message = 64;
  return a;
}

}  // namespace

TEST_CASE("a tape keeps deliveries that changed something and round-trips as text") {
  Tape t;
  t.record(1, 1, {State::connecting, "", std::nullopt}, {});
  t.record(1, 2, {State::connecting, "", std::nullopt}, {});  // nothing new: dropped
  t.record(1, 3, {State::open, "", std::nullopt}, {{"hi", true}, {std::string("\0\xff", 2), false}});
  t.record(1, 9, {State::closed, "boom", 4000}, {});
  CHECK(t.at(1, 2) == nullptr);
  const Tape back = Tape::parse(t.text());
  const Tape::Event* e = back.at(1, 3);
  REQUIRE(e);
  CHECK(e->status.state == State::open);
  REQUIRE(e->messages.size() == 2);
  CHECK(e->messages[0].text);
  CHECK(e->messages[1].data == std::string("\0\xff", 2));
  CHECK(back.at(1, 9)->status.error == "boom");
  CHECK(back.at(1, 9)->status.close_code == 4000);
  CHECK_THROWS_AS(Tape::parse("nope"), alpha::core::Error);
  CHECK_THROWS_AS(Tape::parse("alpha-net-tape 1\nQ 1\n"), alpha::core::Error);
}

TEST_CASE("a recorded run replays the same deliveries with no network") {
  auto tape = std::make_shared<Tape>();
  {
    FakeNet fake;
    const auto srv = fake.server(Kind::ws, endpoint_of(ws_here()), {});
    Hub hub(fake.factory());
    hub.use_tape(tape, false);
    hub.open(ws_here());
    hub.deliver();
    const auto peer = fake.poll(srv).connects.at(0);
    fake.send(peer, {"one", true});
    hub.deliver();
    fake.close(peer, 4001);
    hub.deliver();
  }
  Hub replay([](const Address&) -> std::unique_ptr<Link> { return nullptr; });
  replay.use_tape(std::make_shared<Tape>(Tape::parse(tape->text())), true);
  Socket& s = *replay.find(replay.open(ws_here()));
  replay.deliver();
  CHECK(s.state() == State::open);
  replay.deliver();
  const auto got = s.receive();
  REQUIRE(got.size() == 1);
  CHECK(got[0].data == "one");
  CHECK(got[0].step == 2);
  replay.deliver();
  CHECK(s.state() == State::closed);
  CHECK(s.close_code() == 4001);
}
