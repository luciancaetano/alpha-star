#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "core/glyph_layout.hpp"

// Unicode bidirectional ordering (UAX #9, SheenBidi): which parts of a
// paragraph run right-to-left and in what order its runs are drawn.
namespace alpha::platform {

// Code points [begin, end) at one direction.
struct BidiRun {
  std::size_t begin = 0;
  std::size_t end = 0;
  bool rtl = false;
};

// One paragraph (no '\n'): `codes` must outlive it.
class BidiParagraph {
 public:
  BidiParagraph(const std::u32string& codes, std::size_t begin, std::size_t end, core::TextDir dir);
  ~BidiParagraph();
  BidiParagraph(const BidiParagraph&) = delete;
  BidiParagraph& operator=(const BidiParagraph&) = delete;

  // The paragraph's base direction (auto: its first strong letter; none = left-to-right).
  bool rtl() const;
  // Its runs in logical order (for measuring).
  std::vector<BidiRun> logical_runs() const;
  // The runs of the line [begin, end) in visual order, left to right.
  std::vector<BidiRun> line_runs(std::size_t begin, std::size_t end) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace alpha::platform
