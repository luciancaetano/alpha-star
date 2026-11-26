#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "net/message.hpp"

namespace alpha::net {

// What alpha:net delivered to a game, per socket and step (--net-record), to be
// delivered again with no network (--net-replay): a network bug made repeatable.
// Sockets are named by their id in the hub, which a deterministic game repeats.
class Tape {
 public:
  struct Event {
    std::uint32_t socket = 0;
    std::uint64_t step = 0;
    LinkStatus status;
    std::vector<Message> messages;
  };

  // Keeps a step's delivery when it brought messages or changed the state.
  void record(std::uint32_t socket, std::uint64_t step, const LinkStatus& status, std::vector<Message> messages);
  // nullptr when nothing was recorded for this socket at this step.
  const Event* at(std::uint32_t socket, std::uint64_t step) const;
  // One line per event, then one per message; data in hex.
  std::string text() const;
  // Throws core::Error naming the line that does not parse.
  static Tape parse(std::string_view text);

 private:
  void add(Event e);

  std::vector<Event> events_;
  std::map<std::pair<std::uint32_t, std::uint64_t>, std::size_t> index_;
  std::map<std::uint32_t, LinkStatus> last_;
};

}  // namespace alpha::net
