#pragma once

#include <cstdint>
#include <vector>

namespace alpha::devtools {

// The dev tools' touch-screen toggle: three fingers held still for one second.
// Pure (no SDL): positions are 0..1 across the window, times in milliseconds.
class TouchToggle {
 public:
  static constexpr std::size_t kFingers = 3;
  static constexpr std::uint64_t kHoldMs = 1000;
  static constexpr float kSlop = 0.05f;  // how far a finger may wobble and still be "still"

  void down(std::uint64_t finger, float x, float y, std::uint64_t ms);
  void move(std::uint64_t finger, float x, float y);
  void up(std::uint64_t finger);
  // True once per gesture, when it has been held long enough by `ms`.
  bool poll(std::uint64_t ms);

 private:
  struct Finger {
    std::uint64_t id;
    float x;
    float y;
  };
  std::vector<Finger> fingers_;
  std::uint64_t since_ = 0;
  bool armed_ = false;  // three fingers down, none moved, not fired yet
};

}  // namespace alpha::devtools
