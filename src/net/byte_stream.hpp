#pragma once

#include <string>
#include <string_view>

namespace alpha::net {

enum class StreamState { connecting, open, closed };

// An ordered byte stream (a TCP connection, or TLS over one) that never blocks.
// The links that frame messages sit on top of it; platform/ has the real ones.
class ByteStream {
 public:
  virtual ~ByteStream() = default;
  // Advances connecting and appends what arrived to `in`.
  virtual StreamState pump(std::string& in) = 0;
  // Queues bytes; sent as soon as the stream is open. Failures show on pump.
  virtual void write(std::string_view bytes) = 0;
  // Why the stream closed; empty for a clean close.
  virtual const std::string& error() const = 0;
};

}  // namespace alpha::net
