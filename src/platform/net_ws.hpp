#pragma once

#include <memory>

#include "net/byte_stream.hpp"
#include "net/link.hpp"

namespace alpha::platform {

// net.ws over a byte stream (TCP for ws://, TLS for wss://): the opening
// handshake from net/ws_handshake, then RFC 6455 frames through wslay
// (fragments, ping/pong and close codes handled there).
std::unique_ptr<net::Link> ws_link(const net::Address& address, std::unique_ptr<net::ByteStream> stream);

}  // namespace alpha::platform
