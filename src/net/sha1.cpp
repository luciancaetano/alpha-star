#include "net/sha1.hpp"

namespace alpha::net {

namespace {

std::uint32_t rol(std::uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

struct Round {
  std::uint32_t f, k;
};

Round round_of(std::size_t i, std::uint32_t b, std::uint32_t c, std::uint32_t d) {
  if (i < 20) return {(b & c) | (~b & d), 0x5A827999};
  if (i < 40) return {b ^ c ^ d, 0x6ED9EBA1};
  if (i < 60) return {(b & c) | (b & d) | (c & d), 0x8F1BBCDC};
  return {b ^ c ^ d, 0xCA62C1D6};
}

void block(std::array<std::uint32_t, 5>& h, const std::uint8_t* p) {
  std::array<std::uint32_t, 80> w{};
  for (std::size_t i = 0; i < 16; ++i) {
    w[i] = std::uint32_t{p[4 * i]} << 24 | std::uint32_t{p[4 * i + 1]} << 16 | std::uint32_t{p[4 * i + 2]} << 8 | p[4 * i + 3];
  }
  for (std::size_t i = 16; i < 80; ++i) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
  std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
  for (std::size_t i = 0; i < 80; ++i) {
    const Round r = round_of(i, b, c, d);
    const std::uint32_t t = rol(a, 5) + r.f + e + r.k + w[i];
    e = d, d = c, c = rol(b, 30), b = a, a = t;
  }
  h[0] += a, h[1] += b, h[2] += c, h[3] += d, h[4] += e;
}

}  // namespace

std::array<std::uint8_t, 20> sha1(std::string_view data) {
  std::array<std::uint32_t, 5> h{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
  std::string m(data);
  const std::uint64_t bits = std::uint64_t{data.size()} * 8;
  m.push_back(static_cast<char>(0x80));
  while (m.size() % 64 != 56) m.push_back('\0');
  for (std::size_t i = 8; i-- > 0;) m.push_back(static_cast<char>((bits >> (8 * i)) & 0xFF));
  for (std::size_t at = 0; at < m.size(); at += 64) block(h, reinterpret_cast<const std::uint8_t*>(m.data() + at));
  std::array<std::uint8_t, 20> out{};
  for (std::size_t i = 0; i < 20; ++i) out[i] = static_cast<std::uint8_t>(h[i / 4] >> (24 - 8 * (i % 4)));
  return out;
}

std::string base64(std::string_view bytes) {
  static constexpr char kDigits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  for (std::size_t i = 0; i < bytes.size(); i += 3) {
    const std::size_t n = bytes.size() - i < 3 ? bytes.size() - i : 3;
    std::uint32_t v = 0;
    for (std::size_t j = 0; j < 3; ++j) v = v << 8 | (j < n ? static_cast<std::uint8_t>(bytes[i + j]) : 0u);
    for (std::size_t j = 0; j < 4; ++j) out.push_back(j <= n ? kDigits[(v >> (18 - 6 * j)) & 63] : '=');
  }
  return out;
}

}  // namespace alpha::net
