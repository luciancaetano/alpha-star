#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "net/link.hpp"

namespace alpha::net {

// One alpha:net socket as scripts see it. What arrives is pumped into a
// pending queue at any time; deliver() makes it (and the state) visible at the
// start of a fixed step, so a step only ever sees a stable picture. Sends are
// queued and written by flush() after the step.
class Socket {
 public:
  Socket(const Address& address, std::unique_ptr<Link> link);
  ~Socket();
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  void pump();
  // Returns how many messages it delivered (the newest in the inbox).
  std::size_t deliver(std::uint64_t step);
  // --net-replay: makes a recorded delivery visible instead of the link's.
  void replay(std::uint64_t step, const LinkStatus& status, const std::vector<Message>& messages);
  // Copies of the newest `n` messages not yet received (--net-record).
  std::vector<Message> newest(std::size_t n) const;
  LinkStatus seen() const { return seen_; }
  void flush();
  // false when the outbound queue is full or the socket is closing/closed.
  // Throws core::Error for a message over maxMessageBytes.
  bool send(Message message);
  std::vector<Message> receive();
  void close(int code, const std::string& reason);

  Kind kind() const { return kind_; }
  const std::string& endpoint() const { return endpoint_; }
  State state() const { return seen_.state; }
  const std::string& error() const { return seen_.error; }
  std::optional<int> close_code() const { return seen_.close_code; }
  Stats stats() const;
  // closed, seen by the script and nothing left to receive: safe to forget.
  bool spent() const { return seen_.state == State::closed && inbox_.empty(); }

 private:
  void fail(std::string error);
  void settle(LinkStatus status);

  Kind kind_;
  Framing framing_;
  std::string endpoint_;
  std::size_t max_message_;
  std::unique_ptr<Link> link_;
  std::deque<Message> arrived_, inbox_, outbox_;
  std::size_t arrived_bytes_ = 0, inbox_bytes_ = 0, outbox_bytes_ = 0;
  std::size_t sent_ = 0, received_ = 0;
  LinkStatus live_, seen_;
};

}  // namespace alpha::net
