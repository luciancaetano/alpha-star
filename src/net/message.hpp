#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace alpha::net {

// alpha:net socket kinds and states (alpha.d.ts `Socket.kind` / `SocketState`).
enum class Kind { tcp, udp, ws };
enum class State { connecting, open, closing, closed };

// One whole message: a TCP frame, a UDP datagram or a WebSocket message.
// text: delivered to scripts as a string (WS text, TCP "lines"), else bytes.
struct Message {
  std::string data;
  bool text = false;
  std::uint64_t step = 0;  // the fixed step it was delivered on
};

// Bytes counted per socket (alpha.d.ts `Socket.stats`).
struct Stats {
  std::size_t sent = 0;
  std::size_t received = 0;
  std::size_t queued_out = 0;
  std::size_t queued_in = 0;
};

// What a transport reports on each pump. error is set only for failures.
struct LinkStatus {
  State state = State::connecting;
  std::string error;
  std::optional<int> close_code;  // WebSocket close code
};

const char* kind_name(Kind kind);
const char* state_name(State state);
// "1 MiB", "8 KiB" or "70000 bytes": sizes in agent-facing messages.
std::string size_text(std::size_t bytes);

}  // namespace alpha::net
