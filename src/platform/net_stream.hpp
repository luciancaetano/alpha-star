#pragma once

#include <memory>
#include <string>

#include "net/byte_stream.hpp"

namespace alpha::platform {

// Starts SDL3_net once (net::Hub's real links call it before their first socket).
// Throws core::Error when it cannot start.
void net_start();

// A TCP connection through SDL3_net: DNS and connect run in the background and
// are polled by pump(); timeout_s bounds both together.
std::unique_ptr<net::ByteStream> tcp_stream(const std::string& host, int port, double timeout_s);

}  // namespace alpha::platform
