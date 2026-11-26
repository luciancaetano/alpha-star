#pragma once

#include <memory>
#include <string>

#include "net/byte_stream.hpp"

namespace alpha::platform {

// TLS (Mbed TLS) over another byte stream, for wss://. The server's certificate
// must name `host` and chain to a root the system trusts (platform/net_roots),
// or, with roots_pem, to one of those PEM certificates only (a private CA).
std::unique_ptr<net::ByteStream> tls_stream(std::unique_ptr<net::ByteStream> inner, const std::string& host,
                                            const std::string& roots_pem = "");

}  // namespace alpha::platform
