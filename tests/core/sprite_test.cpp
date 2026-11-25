#include <doctest/doctest.h>

#include <chrono>

#include "core/sheet.hpp"
#include "core/tilemap.hpp"

using namespace alpha::core;

TEST_CASE("sheet frames are row-major cells inside the picture's region") {
  Sheet s;
  s.picture.region = {100, 50, 64, 32};  // e.g. its place in an atlas page
  s.frame_w = 16;
  s.frame_h = 16;
  CHECK(s.columns() == 4);
  CHECK(s.frame_count() == 8);
  CHECK(s.frame(0) == Region{100, 50, 16, 16});
  CHECK(s.frame(5) == Region{116, 66, 16, 16});
}

TEST_CASE("looping animations wrap; one-shots hold their last frame") {
  const Animation run{{4, 5, 6}, 10.0f, true};
  CHECK(frame_at(run, 0.0f) == 0);
  CHECK(frame_at(run, 0.15f) == 1);
  CHECK(frame_at(run, 0.31f) == 0);
  CHECK_FALSE(finished(run, 10.0f));
  const Animation jump{{1, 2}, 4.0f, false};
  CHECK(frame_at(jump, 0.3f) == 1);
  CHECK(frame_at(jump, 9.0f) == 1);
  CHECK_FALSE(finished(jump, 0.1f));
  CHECK(finished(jump, 0.5f));
}

TEST_CASE("frame timing is exact on the fixed step (no float drift at frame edges)") {
  const Animation a{{0, 1, 2, 3}, 10.0f, true};
  float t = 0.0f;
  for (int i = 0; i < 6; ++i) t += 1.0f / 60.0f;  // 6 steps = 0.1 s
  CHECK(frame_at(a, t) == 1);
}

TEST_CASE("merge_cells joins runs and stacks equal spans") {
  // ##..
  // ##..
  // ####
  const std::vector<bool> solid{true, true, false, false, true, true, false, false, true, true, true, true};
  const auto rects = merge_cells(solid, 4);
  REQUIRE(rects.size() == 2);
  CHECK(rects[0] == Region{0, 0, 2, 2});
  CHECK(rects[1] == Region{0, 2, 4, 1});
}

TEST_CASE("merge_cells covers every solid cell exactly once") {
  const std::vector<bool> solid{true, false, true, true, true, false, true, false, true};
  const auto rects = merge_cells(solid, 3);
  std::vector<int> cover(solid.size(), 0);
  for (const Region& r : rects) {
    for (int y = r.y; y < r.y + r.h; ++y) {
      for (int x = r.x; x < r.x + r.w; ++x) ++cover[static_cast<std::size_t>(y * 3 + x)];
    }
  }
  for (std::size_t i = 0; i < solid.size(); ++i) CHECK(cover[i] == (solid[i] ? 1 : 0));
  CHECK(merge_cells({}, 5).empty());
}

TEST_CASE("merge_cells stays linear on a 1000 x 1000 checkerboard (no merges possible)") {
  constexpr int kSide = 1000;
  std::vector<bool> solid(static_cast<std::size_t>(kSide) * kSide);
  for (std::size_t i = 0; i < solid.size(); ++i) solid[i] = ((i % kSide) + (i / kSide)) % 2 == 0;
  const auto start = std::chrono::steady_clock::now();
  const auto rects = merge_cells(solid, kSide);
  CHECK(rects.size() == solid.size() / 2);
  CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(5));  // was minutes (quadratic)
  // Columns of solid cells still stack: a gap row ends them.
  const std::vector<bool> towers{true, false, true, true, false, true, false, false, false, true, false, true};
  const auto t = merge_cells(towers, 3);
  REQUIRE(t.size() == 4);
  CHECK(t[0] == Region{0, 0, 1, 2});
  CHECK(t[1] == Region{2, 0, 1, 2});
  CHECK(t[2] == Region{0, 3, 1, 1});
}
