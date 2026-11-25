#include <doctest/doctest.h>

#include <string>

#include "core/error.hpp"
#include "mcp/socket.hpp"

using alpha::mcp::Socket;

namespace {

// Polls until `sock` has received `want` bytes or a generous number of tries pass.
std::string receive_all(const Socket& sock, std::size_t want) {
  std::string got;
  for (int i = 0; i < 100000 && got.size() < want; ++i) sock.receive(got);
  return got;
}

Socket accept_one(const Socket& server) {
  for (int i = 0; i < 100000; ++i) {
    if (Socket s = server.accept(); s.valid()) return s;
  }
  return {};
}

}  // namespace

TEST_CASE("socket: loopback round trip, non-blocking on both ends") {
  Socket server = Socket::listen(0);
  REQUIRE(server.valid());
  CHECK(server.port() > 0);
  CHECK_FALSE(server.accept().valid());  // nobody waiting: returns at once
  Socket client = Socket::connect(server.port());
  Socket peer = accept_one(server);
  REQUIRE(peer.valid());
  std::string none;
  CHECK(peer.receive(none));  // nothing yet, still open
  CHECK(none.empty());
  const std::string big(50000, 'x');  // many receive chunks, within one socket buffer
  REQUIRE(client.send("hello\n" + big));
  CHECK(receive_all(peer, 6 + big.size()) == "hello\n" + big);
  REQUIRE(peer.send("world\n"));
  CHECK(receive_all(client, 6) == "world\n");
  client = Socket();  // closed by the client
  std::string rest;
  bool open = true;
  for (int i = 0; i < 100000 && open; ++i) open = peer.receive(rest);
  CHECK_FALSE(open);
}

TEST_CASE("socket: a taken port and a closed port are errors, not crashes") {
  Socket server = Socket::listen(0);
  CHECK_THROWS_AS(Socket::listen(server.port()), alpha::core::Error);
  const int port = server.port();
  server = Socket();
  CHECK_THROWS_AS(Socket::connect(port), alpha::core::Error);
}

TEST_CASE("socket: only 127.0.0.0/8 counts as loopback") {
  CHECK(alpha::mcp::is_loopback(0x7F000001));
  CHECK(alpha::mcp::is_loopback(0x7F0A0B0C));
  CHECK_FALSE(alpha::mcp::is_loopback(0xC0A80001));
  CHECK_FALSE(alpha::mcp::is_loopback(0));
}
