#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "net/framing.hpp"
#include "net/message.hpp"

namespace alpha::net {

// Where a socket connects and how, validated at the script boundary.
struct Address {
  Kind kind = Kind::tcp;
  std::string host;
  int port = 0;
  std::string path = "/";  // ws only
  bool tls = false;         // wss://
  Framing framing = Framing::u32;
  std::size_t max_message = 0;
  double connect_timeout = 10;
  bool no_delay = true;
  std::vector<std::string> protocols;                         // ws Sec-WebSocket-Protocol
  std::vector<std::pair<std::string, std::string>> headers;  // ws handshake extras
};

// "host:port" for tcp/udp, the URL for ws: names a socket in errors, MCP and the test fake.
std::string endpoint_of(const Address& address);

// A transport carrying whole messages (real sockets in platform/, a fake in tests).
// Never blocks. Single-threaded: the socket owning it is its only caller.
class Link {
 public:
  virtual ~Link() = default;
  // Appends the whole messages that arrived to `in`; returns the state now.
  virtual LinkStatus pump(std::deque<Message>& in) = 0;
  // Writes what it can from the front of `out`, popping each message written.
  virtual void write(std::deque<Message>& out) = 0;
  // Starts a clean close (pump then reports closing, then closed with no error).
  virtual void close(int code, const std::string& reason) = 0;
};

using LinkFactory = std::function<std::unique_ptr<Link>(const Address&)>;

}  // namespace alpha::net
