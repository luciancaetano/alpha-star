#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "net/link.hpp"

namespace alpha::net {

// WebSocket opening handshake (RFC 6455 section 4), client side. Pure text in,
// text out: the transport and the frames after it live elsewhere.

// ws://host[:port][/path] or wss://... into a ws Address (port 80 / 443, path "/").
// Throws core::Error written for the agent ("net.ws: url must start with ...").
Address parse_ws_url(std::string_view url);
// Names set by the engine itself; a script header with one of them is refused.
bool ws_reserved_header(std::string_view name);
// The HTTP upgrade request for `key` (16 random bytes, base64).
std::string ws_request(const Address& address, const std::string& key);
// The Sec-WebSocket-Accept a server must answer for `key`.
std::string ws_accept(const std::string& key);
// Checks the server's response head (status line + headers, up to the blank line):
// "" when it accepted the upgrade, else why not ("ws handshake failed: ...").
std::string ws_check_response(std::string_view head, const std::string& key, const std::vector<std::string>& protocols);

}  // namespace alpha::net
