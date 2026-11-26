#include <doctest/doctest.h>

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include <functional>
#include <string>

#include "net/framing.hpp"
#include "net/hub.hpp"
#include "net/ws_handshake.hpp"
#include "platform/net_links.hpp"
#include "platform/net_stream.hpp"

using namespace alpha;

namespace {

// Pumps until `done` or 3 s pass (real sockets on the loopback interface).
bool until(const std::function<bool()>& done) {
  const Uint64 end = SDL_GetTicks() + 3000;
  while (!done()) {
    if (SDL_GetTicks() > end) return false;
    SDL_Delay(1);
  }
  return true;
}

// A loopback server on the first free port from 47610; accepts one client.
struct Server {
  NET_Server* server = nullptr;
  NET_StreamSocket* client = nullptr;
  int port = 0;
  Server() {
    platform::net_start();
    NET_Address* local = NET_ResolveHostname("127.0.0.1");
    NET_WaitUntilResolved(local, -1);
    for (port = 47610; !server && port < 47640; ++port) server = NET_CreateServer(local, static_cast<Uint16>(port), 0);
    --port;
    NET_UnrefAddress(local);
  }
  ~Server() {
    if (client) NET_DestroyStreamSocket(client);
    if (server) NET_DestroyServer(server);
  }
  bool accept() { return client || (NET_AcceptClient(server, &client) && client); }
  std::string read() {
    char buf[4096];
    const int n = client ? NET_ReadFromStreamSocket(client, buf, sizeof buf) : 0;
    return n > 0 ? std::string(buf, static_cast<std::size_t>(n)) : std::string();
  }
  void write(const std::string& s) { NET_WriteToStreamSocket(client, s.data(), static_cast<int>(s.size())); }
};

net::Address tcp_to(int port) {
  net::Address a;
  a.host = "127.0.0.1";
  a.port = port;
  a.framing = net::Framing::u32;
  a.max_message = 1024;
  a.connect_timeout = 3;
  return a;
}

// One step: deliver what arrived, then write what was sent.
void step(net::Hub& hub) {
  hub.deliver();
  hub.flush();
}

}  // namespace

TEST_CASE("real tcp: connect, framed messages both ways, peer close") {
  Server srv;
  REQUIRE(srv.server);
  net::Hub hub(platform::real_links());
  net::Socket& s = *hub.find(hub.open(tcp_to(srv.port)));
  CHECK(s.send({"hi", false}));
  REQUIRE(until([&] { step(hub); return srv.accept() && s.state() == net::State::open; }));
  std::string got;
  REQUIRE(until([&] { step(hub); got += srv.read(); return got.size() >= 6; }));
  CHECK(got == net::frame(net::Framing::u32, "hi"));
  srv.write(net::frame(net::Framing::u32, "back"));
  std::vector<net::Message> in;
  REQUIRE(until([&] { step(hub); for (auto& m : s.receive()) in.push_back(m); return !in.empty(); }));
  CHECK(in[0].data == "back");
  NET_DestroyStreamSocket(srv.client);
  srv.client = nullptr;
  REQUIRE(until([&] { step(hub); return s.state() == net::State::closed; }));
  CHECK(s.error().empty());
}

TEST_CASE("real tcp: nothing listening is a refusal naming the endpoint") {
  net::Hub hub(platform::real_links());
  net::Socket& s = *hub.find(hub.open(tcp_to(47609)));
  REQUIRE(until([&] { step(hub); return s.state() == net::State::closed; }));
  CHECK(s.error().find("127.0.0.1:47609") != std::string::npos);
}

TEST_CASE("real ws: handshake, text frames both ways, close code from the server") {
  Server srv;
  REQUIRE(srv.server);
  net::Hub hub(platform::real_links());
  net::Address a = net::parse_ws_url("ws://127.0.0.1:" + std::to_string(srv.port) + "/game");
  a.max_message = 1024;
  a.connect_timeout = 3;
  net::Socket& s = *hub.find(hub.open(a));
  std::string req;
  REQUIRE(until([&] { step(hub); srv.accept(); req += srv.read(); return req.find("\r\n\r\n") != std::string::npos; }));
  CHECK(req.starts_with("GET /game HTTP/1.1\r\n"));
  const auto at = req.find("Sec-WebSocket-Key: ") + 19;
  const std::string key = req.substr(at, req.find("\r\n", at) - at);
  srv.write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
            net::ws_accept(key) + "\r\n\r\n" + std::string("\x81\x05hello", 7));
  std::vector<net::Message> in;
  REQUIRE(until([&] { step(hub); for (auto& m : s.receive()) in.push_back(m); return !in.empty(); }));
  CHECK(s.state() == net::State::open);
  CHECK(in[0].data == "hello");
  CHECK(in[0].text);
  s.send({"yo", true});
  std::string frame;
  REQUIRE(until([&] { step(hub); frame += srv.read(); return frame.size() >= 8; }));
  CHECK(static_cast<unsigned char>(frame[0]) == 0x81);
  CHECK(static_cast<unsigned char>(frame[1]) == (0x80 | 2));  // masked, 2 bytes
  std::string payload = frame.substr(6, 2);
  for (std::size_t i = 0; i < 2; ++i) payload[i] = static_cast<char>(payload[i] ^ frame[2 + i]);
  CHECK(payload == "yo");
  srv.write(std::string("\x88\x02\x0f\xa0", 4));  // close 4000
  REQUIRE(until([&] { step(hub); return s.state() == net::State::closed; }));
  CHECK(s.close_code() == 4000);
  CHECK(s.error().empty());
}

TEST_CASE("real ws: a refused upgrade reports the HTTP status") {
  Server srv;
  REQUIRE(srv.server);
  net::Hub hub(platform::real_links());
  net::Address a = net::parse_ws_url("ws://127.0.0.1:" + std::to_string(srv.port) + "/");
  a.max_message = 64;
  a.connect_timeout = 3;
  net::Socket& s = *hub.find(hub.open(a));
  REQUIRE(until([&] { step(hub); return srv.accept() && !srv.read().empty(); }));
  srv.write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
  REQUIRE(until([&] { step(hub); return s.state() == net::State::closed; }));
  CHECK(s.error() == "ws handshake failed: server answered HTTP 404");
}

TEST_CASE("real udp: datagrams to and from the peer") {
  platform::net_start();
  NET_Address* local = NET_ResolveHostname("127.0.0.1");
  NET_WaitUntilResolved(local, -1);
  NET_DatagramSocket* peer = nullptr;
  int port = 47650;
  for (; !peer && port < 47680; ++port) peer = NET_CreateDatagramSocket(local, static_cast<Uint16>(port), 0);
  --port;
  REQUIRE(peer);
  net::Hub hub(platform::real_links());
  net::Address a = tcp_to(port);
  a.kind = net::Kind::udp;
  net::Socket& s = *hub.find(hub.open(a));
  s.send({"ping", false});
  NET_Datagram* d = nullptr;
  REQUIRE(until([&] { step(hub); return NET_ReceiveDatagram(peer, &d) && d; }));
  CHECK(std::string(reinterpret_cast<char*>(d->buf), static_cast<std::size_t>(d->buflen)) == "ping");
  NET_SendDatagram(peer, d->addr, d->port, "pong", 4);
  NET_DestroyDatagram(d);
  std::vector<net::Message> in;
  REQUIRE(until([&] { step(hub); for (auto& m : s.receive()) in.push_back(m); return !in.empty(); }));
  CHECK(in[0].data == "pong");
  NET_DestroyDatagramSocket(peer);
  NET_UnrefAddress(local);
}

TEST_CASE("real ws: a connection dropped without a close frame is an error") {
  Server srv;
  REQUIRE(srv.server);
  net::Hub hub(platform::real_links());
  net::Address a = net::parse_ws_url("ws://127.0.0.1:" + std::to_string(srv.port) + "/");
  a.max_message = 64;
  a.connect_timeout = 3;
  net::Socket& s = *hub.find(hub.open(a));
  std::string req;
  REQUIRE(until([&] { step(hub); srv.accept(); req += srv.read(); return req.find("\r\n\r\n") != std::string::npos; }));
  const auto at = req.find("Sec-WebSocket-Key: ") + 19;
  srv.write("HTTP/1.1 101 OK\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
            net::ws_accept(req.substr(at, req.find("\r\n", at) - at)) + "\r\n\r\n");
  REQUIRE(until([&] { step(hub); return s.state() == net::State::open; }));
  NET_DestroyStreamSocket(srv.client);
  srv.client = nullptr;
  REQUIRE(until([&] { step(hub); return s.state() == net::State::closed; }));
  CHECK(s.error() == "connection closed without a close frame");
}
