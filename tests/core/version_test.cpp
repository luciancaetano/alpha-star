#include <doctest/doctest.h>

#include "core/version.hpp"

TEST_CASE("version matches the CMake project version") {
  CHECK(alpha::core::version() == ALPHA_VERSION);
  CHECK_FALSE(alpha::core::version().empty());
}
