#include "platform/net_tls.hpp"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/ssl.h>
#include <psa/crypto.h>

#include <algorithm>
#include <array>
#include <utility>

#include "core/error.hpp"
#include "platform/net_roots.hpp"

namespace alpha::platform {

namespace {

std::string tls_text(int code) {
  std::array<char, 160> buf{};
  mbedtls_strerror(code, buf.data(), buf.size());
  return buf.data();
}

// Client config, trusted roots and random generator: the system's roots, shared by
// every wss:// stream (made on the first one; single-threaded here), or one CA's.
struct TlsConfig {
  mbedtls_ssl_config conf;
  mbedtls_x509_crt roots;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context drbg;
  int root_count = 0;

  // roots_pem empty: the system's trusted roots.
  explicit TlsConfig(const std::string& roots_pem) {
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&roots);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&drbg);
    if (psa_crypto_init() != PSA_SUCCESS) throw core::Error("wss: crypto init failed");
    if (const int r = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, nullptr, 0)) throw core::Error("wss: " + tls_text(r));
    mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &drbg);
    root_count = roots_pem.empty() ? load_system_roots(roots) : pem_roots(roots, roots_pem);
    mbedtls_ssl_conf_ca_chain(&conf, &roots, nullptr);
  }
  ~TlsConfig() {
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&roots);
    mbedtls_ctr_drbg_free(&drbg);
    mbedtls_entropy_free(&entropy);
  }
  TlsConfig(const TlsConfig&) = delete;
  TlsConfig& operator=(const TlsConfig&) = delete;

  static std::shared_ptr<TlsConfig> of(const std::string& roots_pem) {
    if (!roots_pem.empty()) return std::make_shared<TlsConfig>(roots_pem);
    static const auto system = std::make_shared<TlsConfig>("");
    return system;
  }
};

class TlsStream final : public net::ByteStream {
 public:
  TlsStream(std::unique_ptr<net::ByteStream> inner, const std::string& host, const std::string& roots_pem)
      : inner_(std::move(inner)), host_(host), config_(TlsConfig::of(roots_pem)) {
    if (config_->root_count == 0) fail("wss: no trusted root certificates found");
    mbedtls_ssl_init(&ssl_);
    mbedtls_ssl_setup(&ssl_, &config_->conf);
    mbedtls_ssl_set_hostname(&ssl_, host.c_str());
    mbedtls_ssl_set_bio(&ssl_, this, send_cb, recv_cb, nullptr);
  }
  ~TlsStream() override {
    if (state_ == net::StreamState::open) mbedtls_ssl_close_notify(&ssl_);
    mbedtls_ssl_free(&ssl_);
  }
  TlsStream(const TlsStream&) = delete;
  TlsStream& operator=(const TlsStream&) = delete;

  net::StreamState pump(std::string& in) override {
    if (state_ == net::StreamState::closed) return state_;
    const net::StreamState below = inner_->pump(cipher_);
    if (state_ == net::StreamState::connecting && below == net::StreamState::open) handshake();
    if (state_ == net::StreamState::open) read(in);
    if (below == net::StreamState::closed && state_ != net::StreamState::closed) {
      fail(inner_->error().empty() ? "connection to " + host_ + " closed during TLS" : inner_->error());
    }
    return state_;
  }
  void write(std::string_view bytes) override {
    if (state_ != net::StreamState::open) return pending_.append(bytes), void();
    while (!bytes.empty() && state_ == net::StreamState::open) {
      const int n = mbedtls_ssl_write(&ssl_, reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
      if (n < 0) return fail("wss: " + tls_text(n));
      bytes.remove_prefix(static_cast<std::size_t>(n));
    }
  }
  const std::string& error() const override { return error_; }

 private:
  void fail(std::string why) {
    state_ = net::StreamState::closed;
    error_ = std::move(why);
  }
  void handshake() {
    const int r = mbedtls_ssl_handshake(&ssl_);
    if (r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE) return;
    if (r != 0) return fail(verify_text(r));
    state_ = net::StreamState::open;
    if (!pending_.empty()) write(std::exchange(pending_, {}));
  }
  std::string verify_text(int r) {
    const std::uint32_t flags = mbedtls_ssl_get_verify_result(&ssl_);
    if (r != MBEDTLS_ERR_X509_CERT_VERIFY_FAILED || flags == 0) return "wss: TLS handshake with " + host_ + " failed: " + tls_text(r);
    std::array<char, 256> buf{};
    mbedtls_x509_crt_verify_info(buf.data(), buf.size(), "", flags);
    return "wss: certificate of " + host_ + " not trusted: " + std::string(buf.data());
  }
  void read(std::string& in) {
    std::array<unsigned char, 16384> buf{};
    for (;;) {
      const int n = mbedtls_ssl_read(&ssl_, buf.data(), buf.size());
      if (n > 0) in.append(reinterpret_cast<const char*>(buf.data()), static_cast<std::size_t>(n));
      if (n > 0 || n == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) continue;
      if (n == MBEDTLS_ERR_SSL_WANT_READ || n == MBEDTLS_ERR_SSL_WANT_WRITE) return;
      return fail(n == 0 || n == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ? "" : "wss: " + tls_text(n));
    }
  }

  static int send_cb(void* user, const unsigned char* buf, std::size_t len) {
    static_cast<TlsStream*>(user)->inner_->write(std::string_view(reinterpret_cast<const char*>(buf), len));
    return static_cast<int>(len);
  }
  static int recv_cb(void* user, unsigned char* buf, std::size_t len) {
    std::string& cipher = static_cast<TlsStream*>(user)->cipher_;
    if (cipher.empty()) return MBEDTLS_ERR_SSL_WANT_READ;
    const std::size_t n = std::min(len, cipher.size());
    std::copy_n(cipher.data(), n, reinterpret_cast<char*>(buf));
    cipher.erase(0, n);
    return static_cast<int>(n);
  }

  std::unique_ptr<net::ByteStream> inner_;
  std::string host_;
  std::shared_ptr<TlsConfig> config_;
  mbedtls_ssl_context ssl_;
  std::string cipher_, pending_, error_;
  net::StreamState state_ = net::StreamState::connecting;
};

}  // namespace

std::unique_ptr<net::ByteStream> tls_stream(std::unique_ptr<net::ByteStream> inner, const std::string& host,
                                            const std::string& roots_pem) {
  return std::make_unique<TlsStream>(std::move(inner), host, roots_pem);
}

}  // namespace alpha::platform
