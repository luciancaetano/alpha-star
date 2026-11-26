#include "net/message.hpp"

namespace alpha::net {

const char* kind_name(Kind kind) {
  switch (kind) {
    case Kind::tcp: return "tcp";
    case Kind::udp: return "udp";
    case Kind::ws: return "ws";
  }
  return "?";
}

const char* state_name(State state) {
  switch (state) {
    case State::connecting: return "connecting";
    case State::open: return "open";
    case State::closing: return "closing";
    case State::closed: return "closed";
  }
  return "?";
}

std::string size_text(std::size_t bytes) {
  constexpr std::size_t kKiB = 1024;
  if (bytes >= kKiB * kKiB && bytes % (kKiB * kKiB) == 0) return std::to_string(bytes / (kKiB * kKiB)) + " MiB";
  if (bytes >= kKiB && bytes % kKiB == 0) return std::to_string(bytes / kKiB) + " KiB";
  return std::to_string(bytes) + " bytes";
}

}  // namespace alpha::net
