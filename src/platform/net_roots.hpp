#pragma once

#include <mbedtls/x509_crt.h>

#include <filesystem>
#include <span>
#include <string>

namespace alpha::platform {

// Adds the system's trusted root certificates to `chain` (Windows: the ROOT
// store; Linux: the distribution bundle; Android: /system/etc/security/cacerts).
// Returns how many were added; 0 means wss:// cannot verify any server.
int load_system_roots(mbedtls_x509_crt& chain);
// Adds the certificates of PEM text (one or many; those that do not parse are skipped).
int pem_roots(mbedtls_x509_crt& chain, const std::string& pem);
// Adds the first readable bundle of `bundles`, else every PEM file under `dir`.
int pem_files_roots(mbedtls_x509_crt& chain, std::span<const char* const> bundles, const std::filesystem::path& dir);

}  // namespace alpha::platform
