#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

#include "net/message.hpp"

namespace alpha::net {

// How TCP bytes become messages: "lines" (UTF-8 ending in \n, \r\n accepted),
// "u16" / "u32" (big-endian payload length first) or "raw" (each chunk read).
enum class Framing { lines, u16, u32, raw };

std::optional<Framing> parse_framing(std::string_view name);
// The largest payload a framing can carry (u16: 65535).
std::size_t framing_cap(Framing framing);
// Throws core::Error (written for socket.send) when a payload cannot be framed.
void check_framable(Framing framing, std::string_view payload);
// The bytes written for one message; check_framable first.
std::string frame(Framing framing, std::string_view payload);

// Splits a received byte stream into messages of at most max_message bytes.
class Deframer {
 public:
  Deframer(Framing framing, std::size_t max_message);
  // Appends whole messages to `out`; returns "" or why the stream must close.
  std::string push(std::string_view bytes, std::deque<Message>& out);

 private:
  std::string lines(std::deque<Message>& out);
  std::string prefixed(std::deque<Message>& out);
  std::string too_big(std::size_t size) const;

  Framing framing_;
  std::size_t max_;
  std::string buffer_;
};

}  // namespace alpha::net
