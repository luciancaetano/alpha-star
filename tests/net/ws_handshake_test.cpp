#include <doctest/doctest.h>

#include "core/error.hpp"
#include "net/sha1.hpp"
#include "net/ws_handshake.hpp"

using namespace alpha::net;

TEST_CASE("sha1 and base64 match known vectors") {
  const auto d = sha1("abc");
  CHECK(base64(std::string_view(reinterpret_cast<const char*>(d.data()), d.size())) == "qZk+NkcGgWq6PiVxeFDCbJzQ2J0=");
  CHECK(base64("") == "");
  CHECK(base64("f") == "Zg==");
  CHECK(base64("fo") == "Zm8=");
  CHECK(base64("foo") == "Zm9v");
}

TEST_CASE("the accept value is the RFC 6455 example") {
  CHECK(ws_accept("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

TEST_CASE("urls parse into host, port, path and tls") {
  const Address a = parse_ws_url("ws://localhost:9000/game?room=1");
  CHECK(a.kind == Kind::ws);
  CHECK(a.host == "localhost");
  CHECK(a.port == 9000);
  CHECK(a.path == "/game?room=1");
  CHECK_FALSE(a.tls);
  const Address b = parse_ws_url("wss://example.com");
  CHECK(b.port == 443);
  CHECK(b.path == "/");
  CHECK(b.tls);
  const Address c = parse_ws_url("ws://[::1]:8080/x");
  CHECK(c.host == "::1");
  CHECK(c.port == 8080);
}

TEST_CASE("bad urls say what was expected and what came") {
  CHECK_THROWS_WITH_AS(parse_ws_url("http://x"), "net.ws: url must start with ws:// or wss://, got \"http://x\"", alpha::core::Error);
  CHECK_THROWS_AS(parse_ws_url("ws://:80/"), alpha::core::Error);
  CHECK_THROWS_AS(parse_ws_url("ws://h:70000/"), alpha::core::Error);
  CHECK_THROWS_AS(parse_ws_url("ws://h:8a/"), alpha::core::Error);
  CHECK_THROWS_AS(parse_ws_url("ws://h/a b"), alpha::core::Error);
}

TEST_CASE("the request carries the key, protocols and extra headers") {
  Address a = parse_ws_url("ws://h:81/p");
  a.protocols = {"chat", "v2"};
  a.headers = {{"Authorization", "Bearer t"}};
  const std::string r = ws_request(a, "KEY");
  CHECK(r.starts_with("GET /p HTTP/1.1\r\nHost: h:81\r\n"));
  CHECK(r.find("Sec-WebSocket-Key: KEY\r\n") != std::string::npos);
  CHECK(r.find("Sec-WebSocket-Protocol: chat, v2\r\n") != std::string::npos);
  CHECK(r.find("Authorization: Bearer t\r\n") != std::string::npos);
  CHECK(r.ends_with("\r\n\r\n"));
  CHECK(ws_request(parse_ws_url("ws://h/"), "K").find("Host: h\r\n") != std::string::npos);
}

TEST_CASE("responses: accepted, refused by status, or a wrong accept") {
  const std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
  const std::string ok = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                         "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n";
  CHECK(ws_check_response(ok, key, {}) == "");
  CHECK(ws_check_response("HTTP/1.1 404 Not Found\r\n", key, {}) == "ws handshake failed: server answered HTTP 404");
  CHECK(ws_check_response(ok, "other", {}) == "ws handshake failed: wrong Sec-WebSocket-Accept");
  CHECK(ws_check_response(ok + "Sec-WebSocket-Protocol: x\r\n", key, {"chat"}).find("not offered") != std::string::npos);
  CHECK(ws_check_response(ok + "Sec-WebSocket-Protocol: chat\r\n", key, {"chat"}) == "");
  CHECK(ws_check_response("garbage", key, {}) == "ws handshake failed: the server did not answer HTTP");
}

TEST_CASE("headers the engine sets itself are reserved") {
  CHECK(ws_reserved_header("Host"));
  CHECK(ws_reserved_header("sec-websocket-key"));
  CHECK(ws_reserved_header("CONNECTION"));
  CHECK_FALSE(ws_reserved_header("Authorization"));
}
