#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include "net/socket.hpp"
#if ALPHA_DEBUG_TOOLS
#include "net/tape.hpp"
#endif

namespace alpha::net {

// Every alpha:net socket of one game instance. Order each fixed step:
// pump() (also on frames the game is paused) -> deliver() -> script -> flush().
// Destroying the hub closes every socket (hot reload, end of a test, quit).
class Hub {
 public:
  explicit Hub(LinkFactory make);
  ~Hub();
  Hub(const Hub&) = delete;
  Hub& operator=(const Hub&) = delete;

  // Throws core::Error past core::kMaxNetSockets sockets not yet closed.
  std::uint32_t open(const Address& address);
  // nullptr once the socket was closed, read and forgotten.
  Socket* find(std::uint32_t id);
  void pump();
  void deliver();
  void flush();
  void close_all();
#if ALPHA_DEBUG_TOOLS
  // --net-record (replay false): every delivery is kept on `tape`. --net-replay:
  // deliveries come from `tape` instead of the links (open them with no network).
  void use_tape(std::shared_ptr<Tape> tape, bool replay);
#endif
  std::uint64_t step() const { return step_; }
  const std::map<std::uint32_t, std::unique_ptr<Socket>>& sockets() const { return sockets_; }

 private:
  LinkFactory make_;
  std::map<std::uint32_t, std::unique_ptr<Socket>> sockets_;
  std::uint32_t next_id_ = 1;
  std::uint64_t step_ = 0;
#if ALPHA_DEBUG_TOOLS
  std::shared_ptr<Tape> tape_;
  bool replay_ = false;
#endif
};

}  // namespace alpha::net
