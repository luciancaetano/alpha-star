#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/random.hpp"
#include "net/link.hpp"

namespace alpha::net {

// In-memory network for test runs (alpha/test `netFake`): servers are named by
// endpoint (endpoint_of), sockets reach them through fake links. Latency is in
// steps (one link pump per fixed step); UDP loss uses a fixed seed, so a test
// sees the same drops on every run. Single-threaded, like everything in net/.
class FakeNet {
 public:
  struct Options {
    int latency_steps = 0;
    double loss = 0;  // UDP only, 0..1
  };
  // What a server saw since its last poll (one poll per fixed step).
  struct Events {
    std::vector<std::uint32_t> connects;
    std::vector<std::pair<std::uint32_t, Message>> messages;
    std::vector<std::uint32_t> closes;
  };
  struct Pipe;

  FakeNet();
  ~FakeNet();
  // Throws core::Error when the endpoint already has a server.
  std::uint32_t server(Kind kind, const std::string& endpoint, Options options);
  Events poll(std::uint32_t server);
  // Throws core::Error for an unknown peer; false once the peer is closed.
  bool send(std::uint32_t peer, Message message);
  void close(std::uint32_t peer, int code);
  LinkFactory factory();

 private:
  struct Server {
    Kind kind;
    Options options;
    std::vector<std::uint32_t> peers;
  };
  std::unique_ptr<Link> connect(const Address& address);
  Pipe& pipe(std::uint32_t peer);
  void poll_peer(std::uint32_t peer, double loss, Events& out);

  std::map<std::string, std::uint32_t> by_endpoint_;
  std::map<std::uint32_t, Server> servers_;
  std::map<std::uint32_t, std::shared_ptr<Pipe>> pipes_;
  std::uint32_t next_id_ = 1;
  core::Random random_{0x6e6574};
};

}  // namespace alpha::net
