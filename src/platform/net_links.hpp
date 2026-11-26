#pragma once

#include "net/link.hpp"

namespace alpha::platform {

// Real network links: tcp (SDL3_net + net::TcpLink), udp (SDL3_net),
// ws (SDL3_net, TLS for wss://, wslay frames). Never blocks.
net::LinkFactory real_links();

}  // namespace alpha::platform
