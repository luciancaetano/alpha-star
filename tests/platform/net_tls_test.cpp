#include <doctest/doctest.h>

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>

#include <array>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#include "platform/net_roots.hpp"
#include "platform/net_stream.hpp"
#include "platform/net_tls.hpp"

using namespace alpha;

namespace {

std::string data_file(const char* name) {
  std::ifstream in(std::string(ALPHA_TEST_TLS_DATA) + "/" + name, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}

bool until(const std::function<bool()>& done) {
  const Uint64 end = SDL_GetTicks() + 5000;
  while (!done()) {
    if (SDL_GetTicks() > end) return false;
    SDL_Delay(1);
  }
  return true;
}

// A TLS server on loopback with Mbed TLS's test certificate (CN=localhost, test-ca2).
struct TlsServer {
  NET_Server* server = nullptr;
  NET_StreamSocket* client = nullptr;
  int port = 0;
  mbedtls_ssl_config conf;
  mbedtls_x509_crt cert;
  mbedtls_pk_context key;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context drbg;
  mbedtls_ssl_context ssl;
  bool shaken = false;

  TlsServer() {
    platform::net_start();
    psa_crypto_init();
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&key);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&drbg);
    mbedtls_ssl_init(&ssl);
    mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, nullptr, 0);
    const std::string crt = data_file("server5.crt"), pem = data_file("server5.key");
    mbedtls_x509_crt_parse(&cert, reinterpret_cast<const unsigned char*>(crt.c_str()), crt.size() + 1);
    mbedtls_pk_parse_key(&key, reinterpret_cast<const unsigned char*>(pem.c_str()), pem.size() + 1, nullptr, 0,
                         mbedtls_ctr_drbg_random, &drbg);
    mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &drbg);
    mbedtls_ssl_conf_own_cert(&conf, &cert, &key);
    mbedtls_ssl_setup(&ssl, &conf);
    mbedtls_ssl_set_bio(&ssl, this, send_cb, recv_cb, nullptr);
    NET_Address* local = NET_ResolveHostname("127.0.0.1");
    NET_WaitUntilResolved(local, -1);
    for (port = 47690; !server && port < 47720; ++port) server = NET_CreateServer(local, static_cast<Uint16>(port), 0);
    --port;
    NET_UnrefAddress(local);
  }
  ~TlsServer() {
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_pk_free(&key);
    mbedtls_x509_crt_free(&cert);
    mbedtls_ctr_drbg_free(&drbg);
    mbedtls_entropy_free(&entropy);
    if (client) NET_DestroyStreamSocket(client);
    if (server) NET_DestroyServer(server);
  }
  // Accepts, then advances the handshake; true once it is done.
  bool handshake() {
    if (!client) NET_AcceptClient(server, &client);
    if (client && !shaken) shaken = mbedtls_ssl_handshake(&ssl) == 0;
    return shaken;
  }
  std::string read() {
    std::array<unsigned char, 256> buf{};
    const int n = mbedtls_ssl_read(&ssl, buf.data(), buf.size());
    return n > 0 ? std::string(reinterpret_cast<char*>(buf.data()), static_cast<std::size_t>(n)) : std::string();
  }
  void write(const std::string& s) { mbedtls_ssl_write(&ssl, reinterpret_cast<const unsigned char*>(s.data()), s.size()); }

  static int send_cb(void* user, const unsigned char* buf, std::size_t len) {
    NET_WriteToStreamSocket(static_cast<TlsServer*>(user)->client, buf, static_cast<int>(len));
    return static_cast<int>(len);
  }
  static int recv_cb(void* user, unsigned char* buf, std::size_t len) {
    const int n = NET_ReadFromStreamSocket(static_cast<TlsServer*>(user)->client, buf, static_cast<int>(len));
    return n == 0 ? MBEDTLS_ERR_SSL_WANT_READ : n < 0 ? MBEDTLS_ERR_SSL_CONN_EOF : n;
  }
};

std::unique_ptr<net::ByteStream> client_to(const TlsServer& srv, const std::string& host, const std::string& roots) {
  return platform::tls_stream(platform::tcp_stream("127.0.0.1", srv.port, 5), host, roots);
}

}  // namespace

TEST_CASE("tls: a trusted server is verified, bytes go both ways, close_notify is a clean close") {
  TlsServer srv;
  REQUIRE(srv.server);
  auto tls = client_to(srv, "localhost", data_file("test-ca2.crt"));
  tls->write("ping");  // queued until the handshake is done
  std::string in, got;
  REQUIRE(until([&] { tls->pump(in); return srv.handshake() && (got += srv.read()) == "ping"; }));
  srv.write("pong");
  REQUIRE(until([&] { tls->pump(in); return in == "pong"; }));
  tls->write("more");
  REQUIRE(until([&] { tls->pump(in); return srv.read() == "more"; }));
  mbedtls_ssl_close_notify(&srv.ssl);
  REQUIRE(until([&] { return tls->pump(in) == net::StreamState::closed; }));
  CHECK(tls->error().empty());
}

TEST_CASE("tls: a certificate from an unknown CA is refused, naming why") {
  TlsServer srv;
  REQUIRE(srv.server);
  auto system = client_to(srv, "localhost", "");  // the test CA is not a system root
  std::string in;
  REQUIRE(until([&] { srv.handshake(); return system->pump(in) == net::StreamState::closed; }));
  CHECK(system->error().find("wss: certificate of localhost not trusted") == 0);
}

TEST_CASE("tls: a certificate for another host is refused") {
  std::string in;
  TlsServer other;
  REQUIRE(other.server);
  auto wrong = client_to(other, "example.com", data_file("test-ca2.crt"));
  REQUIRE(until([&] { other.handshake(); return wrong->pump(in) == net::StreamState::closed; }));
  CHECK(wrong->error().find("wss: certificate of example.com not trusted") == 0);
}

TEST_CASE("tls: no usable roots and a dead connection both close with a reason") {
  std::string in;
  auto none = platform::tls_stream(platform::tcp_stream("127.0.0.1", 47689, 1), "localhost", "not a certificate");
  CHECK(none->pump(in) == net::StreamState::closed);
  CHECK(none->error() == "wss: no trusted root certificates found");
  auto refused = platform::tls_stream(platform::tcp_stream("127.0.0.1", 47689, 2), "localhost", data_file("test-ca2.crt"));
  REQUIRE(until([&] { return refused->pump(in) == net::StreamState::closed; }));
  CHECK(refused->error().find("127.0.0.1:47689") != std::string::npos);
}

TEST_CASE("system roots load, and PEM bundles or directories are read") {
  mbedtls_x509_crt chain;
  mbedtls_x509_crt_init(&chain);
  CHECK(platform::load_system_roots(chain) > 0);
  mbedtls_x509_crt_free(&chain);
  mbedtls_x509_crt_init(&chain);
  const std::string dir = std::string(ALPHA_TEST_TLS_DATA);
  const std::string bundle = dir + "/test-ca2.crt";
  const std::array<const char*, 2> bundles{"/no/such/bundle.pem", bundle.c_str()};
  CHECK(platform::pem_files_roots(chain, bundles, "/no/such/dir") == 1);
  CHECK(platform::pem_files_roots(chain, std::span<const char* const>(), dir + "/dir2") >= 1);
  mbedtls_x509_crt_free(&chain);
}
