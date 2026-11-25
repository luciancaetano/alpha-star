#pragma once

#include <cstdint>

namespace alpha::core {

// Seeded PCG32. The only randomness scripts get, so runs are reproducible.
class Random {
 public:
  explicit Random(std::uint64_t seed);

  std::uint32_t next();
  // Float in [min, max).
  float range(float min, float max);
  // Integer in [min, max]. Requires min <= max.
  std::int32_t integer(std::int32_t min, std::int32_t max);

 private:
  std::uint64_t state_ = 0;
  std::uint64_t inc_ = 0;
};

}  // namespace alpha::core
