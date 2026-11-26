#include "platform/net_roots.hpp"

#include <array>

#include "platform/vfs.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

namespace alpha::platform {

namespace {

int count(const mbedtls_x509_crt& chain) {
  int n = 0;
  for (const mbedtls_x509_crt* c = &chain; c && c->raw.p; c = c->next) ++n;
  return n;
}

#if defined(_WIN32)
// Kept small: only Windows runs (and covers) these two.
int store_roots(mbedtls_x509_crt& chain, HCERTSTORE store) {
  int added = 0;
  for (PCCERT_CONTEXT c = CertEnumCertificatesInStore(store, nullptr); c; c = CertEnumCertificatesInStore(store, c)) {
    added += static_cast<int>(mbedtls_x509_crt_parse_der(&chain, c->pbCertEncoded, c->cbCertEncoded) == 0);
  }
  return added;
}

int windows_roots(mbedtls_x509_crt& chain) {
  HCERTSTORE store = CertOpenSystemStoreW(0, L"ROOT");
  if (!store) return 0;
  const int added = store_roots(chain, store);
  CertCloseStore(store, 0);
  return added;
}
#endif

}  // namespace

int pem_roots(mbedtls_x509_crt& chain, const std::string& pem) {
  const int before = count(chain);
  mbedtls_x509_crt_parse(&chain, reinterpret_cast<const unsigned char*>(pem.c_str()), pem.size() + 1);
  return count(chain) - before;
}

int pem_files_roots(mbedtls_x509_crt& chain, std::span<const char* const> bundles, const std::filesystem::path& dir) {
  for (const char* path : bundles) {
    if (const auto pem = vfs::read_text(path)) return pem_roots(chain, *pem);
  }
  int added = 0;
  for (const auto& [name, info] : vfs::files_under(dir)) {
    if (const auto pem = vfs::read_text(dir / name)) added += pem_roots(chain, *pem);
  }
  return added;
}

int load_system_roots(mbedtls_x509_crt& chain) {
#if defined(_WIN32)
  return windows_roots(chain);
#else
  // Linux distributions' bundles, then Android's directory of one PEM per root.
  static constexpr std::array<const char*, 4> kBundles{
      "/etc/ssl/certs/ca-certificates.crt", "/etc/pki/tls/certs/ca-bundle.crt", "/etc/ssl/ca-bundle.pem", "/etc/ssl/cert.pem"};
  return pem_files_roots(chain, kBundles, "/system/etc/security/cacerts");
#endif
}

}  // namespace alpha::platform
