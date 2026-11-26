#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace alpha::net {

// SHA-1 and base64, only for the WebSocket opening handshake (RFC 6455 4.2.2:
// Sec-WebSocket-Accept = base64(sha1(key + GUID))). Not for anything secret.
std::array<std::uint8_t, 20> sha1(std::string_view data);
std::string base64(std::string_view bytes);

}  // namespace alpha::net
