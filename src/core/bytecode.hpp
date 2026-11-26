#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace alpha::core {

// Which engine build made a game.jsbc, and which one may run it: compared
// exactly (QuickJS does not validate bytecode, so only the same build reads it).
struct BuildStamp {
  std::string engine;   // alpha-star version, e.g. "0.2.0" (<= 12 chars)
  std::string quickjs;  // QuickJS-ng version, e.g. "0.10.1" (<= 12 chars)
  std::string build;    // QuickJS sources hash + pointer width (<= 16 chars)
  bool operator==(const BuildStamp&) const = default;
};

// The per-engine-build key game.jsbc is encrypted with (XChaCha20-Poly1305), shared by
// the release engine (--compile) and the shipping engine of one build. `id` (16 hex
// chars, not secret) names it in the header. Wiped when destroyed.
struct BytecodeKey {
  std::array<std::uint8_t, 32> bytes{};
  std::string id;
  BytecodeKey() = default;
  BytecodeKey(const BytecodeKey&) = delete;
  BytecodeKey& operator=(const BytecodeKey&) = delete;
  ~BytecodeKey();
};
using BytecodeNonce = std::array<std::uint8_t, 24>;

// Decrypted bytecode: lives only in memory and is wiped when destroyed.
struct WipedBytes {
  std::string bytes;
  WipedBytes() = default;
  WipedBytes(const WipedBytes&) = delete;
  WipedBytes& operator=(const WipedBytes&) = delete;
  WipedBytes(WipedBytes&&) = default;
  WipedBytes& operator=(WipedBytes&&) = default;
  ~WipedBytes();
};

// The compiled game in a standalone pack, next to the unmodified shipping engine.
inline constexpr std::string_view kBytecodeFile = "game.jsbc";
// Header, little-endian: magic "ALPHAJBC" | u32 format | u32 body length | key id[16] |
// nonce[24] | Poly1305 tag[16] | engine[12] | quickjs[12] | build[16] (text NUL-padded).
// The body is the encrypted bytecode; the header (tag zeroed) is the associated data.
inline constexpr std::size_t kBytecodeHeader = 112;
inline constexpr std::uint32_t kBytecodeFormat = 2;

// Header + encrypted body.
std::string seal_bytecode(std::string_view body, const BuildStamp& stamp, const BytecodeKey& key,
                          const BytecodeNonce& nonce);
// The decrypted body of `file` when its header is stamped by `self`, encrypted with `key`
// and authentic; throws core::Error otherwise (naming both builds when they differ).
WipedBytes open_bytecode(std::string_view file, const BuildStamp& self, const BytecodeKey& key);
// The stamp in a header (non-printable bytes as '?'); needs kBytecodeHeader bytes.
BuildStamp stamp_of(std::string_view file);
// "alpha-star 0.2.0 (QuickJS 0.10.1, build ...)".
std::string describe(const BuildStamp& stamp);

}  // namespace alpha::core
