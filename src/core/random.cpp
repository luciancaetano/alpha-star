#include "core/random.hpp"

namespace alpha::core {

Random::Random(std::uint64_t seed) : inc_((seed << 1u) | 1u) {
  next();
  state_ += seed;
  next();
}

std::uint32_t Random::next() {
  const std::uint64_t old = state_;
  state_ = old * 6364136223846793005ULL + inc_;
  const auto shifted = static_cast<std::uint32_t>(((old >> 18u) ^ old) >> 27u);
  const auto rot = static_cast<std::uint32_t>(old >> 59u);
  return (shifted >> rot) | (shifted << ((32u - rot) & 31u));
}

float Random::range(float min, float max) {
  const float unit = static_cast<float>(next() >> 8) / 16777216.0f;  // 24-bit mantissa
  return min + (max - min) * unit;
}

std::int32_t Random::integer(std::int32_t min, std::int32_t max) {
  const auto span = static_cast<std::uint64_t>(static_cast<std::int64_t>(max) - min) + 1u;
  return static_cast<std::int32_t>(min + static_cast<std::int64_t>(next() % span));
}

}  // namespace alpha::core
