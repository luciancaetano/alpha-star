#include "mcp/socket.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <utility>

#include "core/error.hpp"

namespace alpha::mcp {

namespace {


#ifdef _WIN32
using Native = SOCKET;
using Length = int;
using Size = int;  // Winsock buffer lengths
constexpr int kSendFlags = 0;

// Winsock needs one WSAStartup per process before any socket call.
void start_network() {
  static const bool started = [] {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
  }();
  if (!started) throw core::Error("control channel: Winsock did not start");
}
void close_native(Native s) { closesocket(s); }
bool set_non_blocking(Native s) {
  u_long on = 1;
  return ioctlsocket(s, FIONBIO, &on) == 0;
}
bool would_block() { return WSAGetLastError() == WSAEWOULDBLOCK; }
int wait_writable(Native s, int ms) {
  WSAPOLLFD p{s, POLLWRNORM, 0};
  return WSAPoll(&p, 1, ms);
}
// Nobody else may bind the same port while we listen (port hijacking).
void exclusive(Native s) {
  BOOL on = TRUE;
  setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&on), static_cast<int>(sizeof on));
}
#else
using Native = int;
using Length = socklen_t;
using Size = std::size_t;
constexpr int kSendFlags = MSG_NOSIGNAL;  // a closed peer is an error, not SIGPIPE

void start_network() {}
void close_native(Native s) { ::close(s); }
bool set_non_blocking(Native s) { return fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK) == 0; }
bool would_block() { return errno == EAGAIN || errno == EWOULDBLOCK; }
int wait_writable(Native s, int ms) {
  pollfd p{s, POLLOUT, 0};
  return poll(&p, 1, ms);
}
void exclusive(Native) {}  // Linux refuses a second bind without SO_REUSEPORT
#endif

constexpr int kSendWaitMs = 5000;

Native native(std::intptr_t fd) { return static_cast<Native>(fd); }

sockaddr_in loopback(int port) {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<std::uint16_t>(port));
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  return addr;
}

Native open_tcp() {
  start_network();
  const Native s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == static_cast<Native>(-1)) throw core::Error("control channel: cannot create a socket");
  return s;
}

[[noreturn]] void fail(Native s, const std::string& what) {
  close_native(s);
  throw core::Error("control channel: " + what);
}

}  // namespace

Socket::~Socket() {
  if (valid()) close_native(native(fd_));
}

Socket::Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, kInvalid)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
  Socket gone(std::exchange(fd_, std::exchange(other.fd_, kInvalid)));
  return *this;
}

Socket Socket::listen(int port) {
  const Native s = open_tcp();
  exclusive(s);
  const sockaddr_in addr = loopback(port);
  if (::bind(s, reinterpret_cast<const sockaddr*>(&addr), static_cast<Length>(sizeof addr)) != 0) {
    fail(s, "cannot listen on 127.0.0.1:" + std::to_string(port) + " (port in use?)");
  }
  if (::listen(s, 4) != 0 || !set_non_blocking(s)) fail(s, "cannot listen");
  return Socket(static_cast<std::intptr_t>(s));
}

namespace {

// Small request/response lines: without this, Nagle holds a second small reply until
// the peer's delayed ACK (40 ms or more on Linux) and pipelined answers stall.
void no_delay(Native s) {
  const int on = 1;
  setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), static_cast<Length>(sizeof on));
}

}  // namespace

Socket Socket::connect(int port) {
  const Native s = open_tcp();
  const sockaddr_in addr = loopback(port);
  if (::connect(s, reinterpret_cast<const sockaddr*>(&addr), static_cast<Length>(sizeof addr)) != 0 || !set_non_blocking(s)) {
    fail(s, "cannot connect to 127.0.0.1:" + std::to_string(port));
  }
  no_delay(s);
  return Socket(static_cast<std::intptr_t>(s));
}

int Socket::port() const {
  sockaddr_in addr{};
  auto len = static_cast<Length>(sizeof addr);
  getsockname(native(fd_), reinterpret_cast<sockaddr*>(&addr), &len);
  return ntohs(addr.sin_port);
}

Socket Socket::accept() const {
  sockaddr_in peer{};
  auto len = static_cast<Length>(sizeof peer);
  const Native s = ::accept(native(fd_), reinterpret_cast<sockaddr*>(&peer), &len);
  if (s == static_cast<Native>(-1)) return {};
  Socket client(static_cast<std::intptr_t>(s));
  if (!is_loopback(ntohl(peer.sin_addr.s_addr)) || !set_non_blocking(s)) return {};
  no_delay(s);
  return client;
}

bool Socket::receive(std::string& into) const {
  char buf[16384];
  for (;;) {
    const auto n = ::recv(native(fd_), buf, static_cast<Size>(sizeof buf), 0);
    if (n > 0) into.append(buf, static_cast<std::size_t>(n));
    else return n < 0 && would_block();
  }
}

bool Socket::send(std::string_view data) const {
  while (!data.empty()) {
    const auto n = ::send(native(fd_), data.data(), static_cast<Size>(data.size()), kSendFlags);
    if (n > 0) data.remove_prefix(static_cast<std::size_t>(n));
    else if (n < 0 && would_block() && wait_writable(native(fd_), kSendWaitMs) > 0) continue;
    else return false;
  }
  return true;
}

}  // namespace alpha::mcp
