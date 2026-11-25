#pragma once

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

// The compiled game in a standalone pack, next to the unmodified shipping engine.
inline constexpr std::string_view kBytecodeFile = "game.jsbc";
// Header, little-endian: magic "ALPHAJBC" | u32 format | u32 body length |
// u64 FNV-1a of the body | engine[12] | quickjs[12] | build[16] (NUL-padded).
inline constexpr std::size_t kBytecodeHeader = 64;
inline constexpr std::uint32_t kBytecodeFormat = 1;

// Header + body.
std::string seal_bytecode(std::string_view body, const BuildStamp& stamp);
// The body of `file` when its header is intact and stamped by `self`; throws
// core::Error otherwise, naming both builds when they differ.
std::string_view open_bytecode(std::string_view file, const BuildStamp& self);
// The stamp in a header (non-printable bytes as '?'); needs kBytecodeHeader bytes.
BuildStamp stamp_of(std::string_view file);
// "alpha-star 0.2.0 (QuickJS 0.10.1, build ...)".
std::string describe(const BuildStamp& stamp);

}  // namespace alpha::core
