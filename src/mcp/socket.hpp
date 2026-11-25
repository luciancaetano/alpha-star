#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace alpha::mcp {

// A TCP socket on the loopback interface only (Winsock / BSD sockets behind
// one type), non-blocking so the main loop can poll it between frames
// without threads. Move-only; closes on destruction.
class Socket {
 public:
  Socket() = default;
  ~Socket();
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  // Listens on 127.0.0.1:port (0 = any free port). Throws core::Error.
  static Socket listen(int port);
  // Connects to 127.0.0.1:port, then turns non-blocking. Throws core::Error.
  static Socket connect(int port);

  bool valid() const { return fd_ != kInvalid; }
  // The local port (useful after listen(0)).
  int port() const;
  // The next pending connection from a loopback address, or an invalid socket
  // when none is waiting. Connections from anywhere else are closed unread.
  Socket accept() const;
  // Appends whatever bytes have arrived; false once the peer closed or failed.
  bool receive(std::string& into) const;
  // Sends everything, waiting (up to a few seconds) while the buffer is full;
  // false on failure.
  bool send(std::string_view data) const;

 private:
  static constexpr std::intptr_t kInvalid = -1;
  explicit Socket(std::intptr_t fd) : fd_(fd) {}
  std::intptr_t fd_ = kInvalid;
};

// True for 127.0.0.0/8 (host byte order).
constexpr bool is_loopback(std::uint32_t ipv4) { return (ipv4 >> 24) == 127; }

}  // namespace alpha::mcp
