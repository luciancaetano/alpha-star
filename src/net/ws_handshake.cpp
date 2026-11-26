#include "net/ws_handshake.hpp"

#include <algorithm>
#include <cctype>
#include <map>

#include "core/error.hpp"
#include "net/sha1.hpp"

namespace alpha::net {

namespace {

std::string lower(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (const char c : s) out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  return out;
}

std::string_view trim(std::string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);
  return s;
}

[[noreturn]] void bad_url(std::string_view url, const char* why) {
  throw core::Error(std::string("net.ws: ") + why + ", got \"" + std::string(url) + "\"");
}

// "host", "host:port", "[v6]" or "[v6]:port": sets the host, returns the port text.
std::string_view split_host(std::string_view url, std::string_view auth, Address& a) {
  std::string_view port;
  if (auth.starts_with('[')) {
    const auto end = auth.find(']');
    if (end == std::string_view::npos) bad_url(url, "url has an unclosed [ in its host");
    a.host = std::string(auth.substr(1, end - 1));
    if (end + 1 < auth.size() && auth[end + 1] == ':') port = auth.substr(end + 2);
  } else {
    const auto colon = auth.find(':');
    a.host = std::string(auth.substr(0, colon));
    if (colon != std::string_view::npos) port = auth.substr(colon + 1);
  }
  if (a.host.empty()) bad_url(url, "url has no host");
  return port;
}

void parse_authority(std::string_view url, std::string_view auth, Address& a) {
  const std::string_view port = split_host(url, auth, a);
  if (port.empty()) return;
  int n = 0;
  for (char c : port) n = std::isdigit(static_cast<unsigned char>(c)) && n < 100000 ? n * 10 + (c - '0') : 1 << 20;
  if (n < 1 || n > 65535) bad_url(url, "url port must be an integer 1..65535");
  a.port = n;
}

// Header names (lowercase) to values; the status line is skipped.
std::map<std::string, std::string> headers_of(std::string_view head) {
  std::map<std::string, std::string> out;
  for (std::size_t at = head.find("\r\n"); at != std::string_view::npos && at + 2 < head.size();) {
    const std::size_t end = std::min(head.find("\r\n", at + 2), head.size());
    const std::string_view line = head.substr(at + 2, end - at - 2);
    const auto colon = line.find(':');
    if (colon != std::string_view::npos) out[lower(trim(line.substr(0, colon)))] = std::string(trim(line.substr(colon + 1)));
    at = end;
  }
  return out;
}

std::string status_error(std::string_view head) {
  const std::string_view line = head.substr(0, head.find("\r\n"));
  if (!line.starts_with("HTTP/1.1 ") || line.size() < 12) return "ws handshake failed: the server did not answer HTTP";
  const std::string_view code = line.substr(9, 3);
  return code == "101" ? "" : "ws handshake failed: server answered HTTP " + std::string(code);
}

}  // namespace

Address parse_ws_url(std::string_view url) {
  Address a;
  a.kind = Kind::ws;
  std::string_view rest;
  if (url.starts_with("ws://")) {
    rest = url.substr(5), a.port = 80;
  } else if (url.starts_with("wss://")) {
    rest = url.substr(6), a.port = 443, a.tls = true;
  } else {
    bad_url(url, "url must start with ws:// or wss://");
  }
  const auto slash = rest.find_first_of("/?");
  parse_authority(url, rest.substr(0, slash), a);
  if (slash != std::string_view::npos) a.path = std::string(rest.substr(slash));
  if (a.path.starts_with('?')) a.path.insert(0, "/");
  if (a.path.find_first_of(" \r\n#") != std::string::npos) bad_url(url, "url path cannot contain spaces, line breaks or #");
  return a;
}

bool ws_reserved_header(std::string_view name) {
  const std::string n = lower(name);
  return n == "host" || n == "upgrade" || n == "connection" || n.starts_with("sec-websocket-") || n == "content-length";
}

std::string ws_request(const Address& a, const std::string& key) {
  const bool v6 = a.host.find(':') != std::string::npos;
  const bool default_port = a.port == (a.tls ? 443 : 80);
  std::string host = v6 ? "[" + a.host + "]" : a.host;
  if (!default_port) host += ":" + std::to_string(a.port);
  std::string r = "GET " + a.path + " HTTP/1.1\r\nHost: " + host +
                  "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: " + key + "\r\n";
  if (!a.protocols.empty()) {
    r += "Sec-WebSocket-Protocol: ";
    for (std::size_t i = 0; i < a.protocols.size(); ++i) r += (i ? ", " : "") + a.protocols[i];
    r += "\r\n";
  }
  for (const auto& [name, value] : a.headers) r += name + ": " + value + "\r\n";
  return r + "\r\n";
}

std::string ws_accept(const std::string& key) {
  const auto digest = sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  return base64(std::string_view(reinterpret_cast<const char*>(digest.data()), digest.size()));
}

std::string ws_check_response(std::string_view head, const std::string& key, const std::vector<std::string>& protocols) {
  if (std::string error = status_error(head); !error.empty()) return error;
  auto h = headers_of(head);
  if (lower(h["upgrade"]) != "websocket") return "ws handshake failed: the server did not upgrade to websocket";
  if (lower(h["connection"]).find("upgrade") == std::string::npos) return "ws handshake failed: Connection is not Upgrade";
  if (h["sec-websocket-accept"] != ws_accept(key)) return "ws handshake failed: wrong Sec-WebSocket-Accept";
  const std::string& chosen = h["sec-websocket-protocol"];
  if (!chosen.empty() && std::find(protocols.begin(), protocols.end(), chosen) == protocols.end()) {
    return "ws handshake failed: the server chose protocol \"" + chosen + "\", which was not offered";
  }
  return "";
}

}  // namespace alpha::net
