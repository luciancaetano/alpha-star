#include <doctest/doctest.h>

#include <string>

#include "mcp/state_json.hpp"
#include "net/fake_net.hpp"

using namespace alpha;

TEST_CASE("state.net lists each socket with its kind, endpoint, state, error and bytes") {
  CHECK(mcp::net_json(nullptr) == "[]");
  net::FakeNet fake;
  fake.server(net::Kind::ws, "ws://h:1/", {});
  net::Hub hub(fake.factory());
  net::Address ws;
  ws.kind = net::Kind::ws;
  ws.host = "h";
  ws.port = 1;
  ws.max_message = 64;
  hub.open(ws);
  net::Address tcp = ws;
  tcp.kind = net::Kind::tcp;
  tcp.port = 2;
  hub.open(tcp);  // no server: refused
  hub.deliver();
  hub.find(1)->send({"hi", true});
  const std::string out = mcp::net_json(&hub);
  CHECK(out.find(R"("kind":"ws")") != std::string::npos);
  CHECK(out.find(R"("endpoint":"ws://h:1/")") != std::string::npos);
  CHECK(out.find(R"("state":"open")") != std::string::npos);
  CHECK(out.find(R"("queuedOut":2)") != std::string::npos);
  CHECK(out.find(R"x("error":"connect to h:2 refused (no netFake server there)")x") != std::string::npos);
}
