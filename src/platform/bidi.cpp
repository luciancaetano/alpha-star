#include "platform/bidi.hpp"

#include <SheenBidi/SheenBidi.h>

namespace alpha::platform {

namespace {

SBLevel base_level(core::TextDir dir) {
  if (dir == core::TextDir::Ltr) return 0;
  if (dir == core::TextDir::Rtl) return 1;
  return SBLevelDefaultLTR;
}

}  // namespace

struct BidiParagraph::Impl {
  std::size_t begin = 0;
  SBAlgorithmRef algorithm = nullptr;
  SBParagraphRef paragraph = nullptr;
};

BidiParagraph::BidiParagraph(const std::u32string& codes, std::size_t begin, std::size_t end, core::TextDir dir)
    : impl_(std::make_unique<Impl>()) {
  impl_->begin = begin;
  if (end <= begin) return;  // an empty paragraph has no runs
  const SBCodepointSequence sequence{SBStringEncodingUTF32, const_cast<char32_t*>(codes.data() + begin), end - begin};
  impl_->algorithm = SBAlgorithmCreate(&sequence);
  impl_->paragraph = SBAlgorithmCreateParagraph(impl_->algorithm, 0, end - begin, base_level(dir));
}

BidiParagraph::~BidiParagraph() {
  if (impl_->paragraph) SBParagraphRelease(impl_->paragraph);
  if (impl_->algorithm) SBAlgorithmRelease(impl_->algorithm);
}

bool BidiParagraph::rtl() const { return impl_->paragraph && (SBParagraphGetBaseLevel(impl_->paragraph) & 1) != 0; }

std::vector<BidiRun> BidiParagraph::logical_runs() const {
  std::vector<BidiRun> out;
  if (!impl_->paragraph) return out;
  const SBLevel* levels = SBParagraphGetLevelsPtr(impl_->paragraph);
  const std::size_t length = SBParagraphGetLength(impl_->paragraph);
  for (std::size_t i = 0; i < length; ++i) {
    const bool rtl = (levels[i] & 1) != 0;
    if (out.empty() || out.back().rtl != rtl) out.push_back({impl_->begin + i, impl_->begin + i, rtl});
    out.back().end = impl_->begin + i + 1;
  }
  return out;
}

std::vector<BidiRun> BidiParagraph::line_runs(std::size_t begin, std::size_t end) const {
  std::vector<BidiRun> out;
  if (!impl_->paragraph || end <= begin) return out;
  const SBLineRef line = SBParagraphCreateLine(impl_->paragraph, begin - impl_->begin, end - begin);
  const SBRun* runs = SBLineGetRunsPtr(line);
  for (std::size_t r = 0; r < SBLineGetRunCount(line); ++r) {
    const std::size_t first = impl_->begin + runs[r].offset;
    out.push_back({first, first + runs[r].length, (runs[r].level & 1) != 0});
  }
  SBLineRelease(line);
  return out;
}

}  // namespace alpha::platform
