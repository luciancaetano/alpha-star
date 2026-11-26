#include "platform/net_stream.hpp"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include <array>

#include "core/error.hpp"

namespace alpha::platform {

void net_start() {
  static const bool started = NET_Init();
  if (!started) throw core::Error(std::string("network unavailable: ") + SDL_GetError());
}

namespace {

class SdlStream final : public net::ByteStream {
 public:
  SdlStream(const std::string& host, int port, double timeout_s)
      : host_(host), port_(port), deadline_(SDL_GetTicks() + static_cast<Uint64>(timeout_s * 1000)), timeout_s_(timeout_s),
        address_(NET_ResolveHostname(host.c_str())) {
    if (!address_) fail("dns lookup failed for \"" + host + "\"");
  }
  ~SdlStream() override {
    if (socket_) NET_DestroyStreamSocket(socket_);
    if (address_) NET_UnrefAddress(address_);
  }
  SdlStream(const SdlStream&) = delete;
  SdlStream& operator=(const SdlStream&) = delete;

  net::StreamState pump(std::string& in) override {
    if (state_ == net::StreamState::connecting) connect();
    if (state_ == net::StreamState::open) read(in);
    return state_;
  }
  void write(std::string_view bytes) override {
    if (state_ == net::StreamState::closed) return;
    if (state_ == net::StreamState::connecting) {
      pending_ += bytes;
    } else if (!NET_WriteToStreamSocket(socket_, bytes.data(), static_cast<int>(bytes.size()))) {
      fail("connection to " + endpoint() + " lost: " + SDL_GetError());
    }
  }
  const std::string& error() const override { return error_; }

 private:
  std::string endpoint() const { return host_ + ":" + std::to_string(port_); }
  void fail(std::string why) {
    state_ = net::StreamState::closed;
    error_ = std::move(why);
  }

  void connect() {
    if (!socket_) resolve();
    if (socket_) connecting();
    if (state_ == net::StreamState::connecting && SDL_GetTicks() > deadline_) {
      fail("connect to " + endpoint() + " timed out after " + std::to_string(static_cast<int>(timeout_s_)) + " s");
    }
  }
  void resolve() {
    const NET_Status s = NET_GetAddressStatus(address_);
    if (s == NET_FAILURE) return fail("dns lookup failed for \"" + host_ + "\"");
    if (s == NET_SUCCESS) socket_ = NET_CreateClient(address_, static_cast<Uint16>(port_), 0);
    if (s == NET_SUCCESS && !socket_) fail("connect to " + endpoint() + " failed: " + SDL_GetError());
  }
  void connecting() {
    const NET_Status s = NET_GetConnectionStatus(socket_);
    if (s == NET_FAILURE) return fail("connect to " + endpoint() + " refused");
    if (s != NET_SUCCESS) return;
    state_ = net::StreamState::open;
    if (!pending_.empty()) write(std::exchange(pending_, {}));
  }
  void read(std::string& in) {
    std::array<char, 16384> buf{};
    for (;;) {
      const int n = NET_ReadFromStreamSocket(socket_, buf.data(), static_cast<int>(buf.size()));
      if (n < 0) return fail(peer_closed() ? "" : "connection to " + endpoint() + " lost: " + SDL_GetError());
      if (n == 0) return;
      in.append(buf.data(), static_cast<std::size_t>(n));
    }
  }
  // SDL3_net reports an orderly shutdown by the peer as a read failure ("End of stream").
  static bool peer_closed() { return std::string_view(SDL_GetError()) == "End of stream"; }

  std::string host_;
  int port_;
  Uint64 deadline_;
  double timeout_s_;
  NET_Address* address_ = nullptr;
  NET_StreamSocket* socket_ = nullptr;
  net::StreamState state_ = net::StreamState::connecting;
  std::string pending_, error_;
};

}  // namespace

std::unique_ptr<net::ByteStream> tcp_stream(const std::string& host, int port, double timeout_s) {
  net_start();
  return std::make_unique<SdlStream>(host, port, timeout_s);
}

}  // namespace alpha::platform
