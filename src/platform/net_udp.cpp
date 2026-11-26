#include "platform/net_udp.hpp"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include "platform/net_stream.hpp"

namespace alpha::platform {

namespace {

class UdpLink final : public net::Link {
 public:
  explicit UdpLink(const net::Address& a)
      : host_(a.host), port_(static_cast<Uint16>(a.port)), address_(NET_ResolveHostname(a.host.c_str())),
        socket_(NET_CreateDatagramSocket(nullptr, 0, 0)) {
    if (!address_) fail("dns lookup failed for \"" + host_ + "\"");
    if (!socket_) fail(std::string("udp socket failed: ") + SDL_GetError());
  }
  ~UdpLink() override {
    if (socket_) NET_DestroyDatagramSocket(socket_);
    if (address_) NET_UnrefAddress(address_);
  }
  UdpLink(const UdpLink&) = delete;
  UdpLink& operator=(const UdpLink&) = delete;

  net::LinkStatus pump(std::deque<net::Message>& in) override {
    if (status_.state == net::State::open && !resolved_) resolve();
    for (NET_Datagram* d = nullptr; status_.state == net::State::open && receive(&d) && d;) {
      if (d->port == port_ && NET_CompareAddresses(d->addr, address_) == 0) {
        in.push_back({std::string(reinterpret_cast<const char*>(d->buf), static_cast<std::size_t>(d->buflen)), false});
      }
      NET_DestroyDatagram(d);
      d = nullptr;
    }
    return status_;
  }
  void write(std::deque<net::Message>& out) override {
    if (!resolved_) return;  // kept until DNS answers
    for (; !out.empty(); out.pop_front()) {
      const std::string& data = out.front().data;
      if (!NET_SendDatagram(socket_, address_, port_, data.data(), static_cast<int>(data.size()))) {
        return fail("udp send to " + host_ + ":" + std::to_string(port_) + " failed: " + SDL_GetError());
      }
    }
  }
  void close(int, const std::string&) override { status_ = {net::State::closed, "", std::nullopt}; }

 private:
  void fail(std::string why) { status_ = {net::State::closed, std::move(why), std::nullopt}; }
  void resolve() {
    const NET_Status s = NET_GetAddressStatus(address_);
    if (s == NET_FAILURE) fail("dns lookup failed for \"" + host_ + "\"");
    resolved_ = s == NET_SUCCESS;
  }
  bool receive(NET_Datagram** d) {
    if (NET_ReceiveDatagram(socket_, d)) return true;
    fail(std::string("udp receive failed: ") + SDL_GetError());
    return false;
  }

  std::string host_;
  Uint16 port_;
  NET_Address* address_;
  NET_DatagramSocket* socket_;
  bool resolved_ = false;
  net::LinkStatus status_{net::State::open, "", std::nullopt};
};

}  // namespace

std::unique_ptr<net::Link> udp_link(const net::Address& address) {
  net_start();
  return std::make_unique<UdpLink>(address);
}

}  // namespace alpha::platform
