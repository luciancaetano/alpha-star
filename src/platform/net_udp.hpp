#pragma once

#include <memory>

#include "net/link.hpp"

namespace alpha::platform {

// net.udp through SDL3_net: datagrams to and from host:port only (others are
// dropped). Open at once; sends wait in the socket's queue while DNS resolves.
std::unique_ptr<net::Link> udp_link(const net::Address& address);

}  // namespace alpha::platform
