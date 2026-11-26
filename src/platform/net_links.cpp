#include "platform/net_links.hpp"

#include "net/tcp_link.hpp"
#include "platform/net_stream.hpp"
#include "platform/net_tls.hpp"
#include "platform/net_udp.hpp"
#include "platform/net_ws.hpp"

namespace alpha::platform {

namespace {

std::unique_ptr<net::Link> make_link(const net::Address& a) {
  if (a.kind == net::Kind::udp) return udp_link(a);
  auto stream = tcp_stream(a.host, a.port, a.connect_timeout);
  if (a.kind == net::Kind::tcp) return std::make_unique<net::TcpLink>(std::move(stream), a.framing, a.max_message);
  if (a.tls) stream = tls_stream(std::move(stream), a.host);
  return ws_link(a, std::move(stream));
}

}  // namespace

net::LinkFactory real_links() { return make_link; }

}  // namespace alpha::platform
