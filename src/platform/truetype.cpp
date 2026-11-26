#include "platform/truetype.hpp"

#include <hb.h>

#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "core/utf8.hpp"
#include "platform/glyphs.hpp"
#include "platform/run_items.hpp"

namespace alpha::platform {

namespace {

using Chain = std::vector<std::shared_ptr<const FontFile>>;

vfs::Bytes read_font(const std::filesystem::path& file) {
  auto bytes = vfs::read(file);
  if (!bytes) {
    throw core::Error("font \"" + file.generic_string() + "\": cannot read the file (paths are relative to the game's main file)");
  }
  return std::move(*bytes);
}

// A box for code points no font has: an outline about half an em wide.
GlyphBitmap tofu(int pixels, float ascent) {
  const int w = std::max(3, static_cast<int>(std::lround(pixels * 0.45f)));
  const int h = std::max(4, static_cast<int>(std::lround(ascent * 0.8f)));
  const int line = std::max(1, pixels / 16);
  GlyphBitmap b{w, h, std::vector<std::uint8_t>(static_cast<std::size_t>(w * h), 0), {}, 0.0f};
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const bool edge = x < line || y < line || x >= w - line || y >= h - line;
      if (edge) b.coverage[static_cast<std::size_t>(y * w + x)] = 255;
    }
  }
  const float margin = std::max(1.0f, std::round(pixels * 0.06f));
  b.offset = {margin, ascent - static_cast<float>(h)};
  b.advance = static_cast<float>(w) + margin * 2.0f;
  return b;
}

class ChainShaper final : public RunShaper {
 public:
  ChainShaper(const Chain& chain, MissingGlyphs& missing) : chain_(chain), missing_(missing) {}
  std::vector<RunGlyph> shape_run(const std::u32string& codes, std::size_t begin, std::size_t end, bool rtl,
                                  int pixels) override {
    const auto fonts = static_cast<std::uint32_t>(chain_.size());
    std::vector<RunItem> items =
        run_items(codes, begin, end, fonts, [this](std::uint32_t f, char32_t c) { return chain_[f]->glyph_of(c) != 0; });
    if (rtl) std::reverse(items.begin(), items.end());
    std::vector<RunGlyph> out;
    for (const RunItem& item : items) {
      std::vector<RunGlyph> glyphs = item.font == kNoFont ? boxes(codes, item, pixels)
                                                          : chain_[item.font]->shape(codes, item.begin, item.end, rtl, pixels, item.font);
      out.insert(out.end(), glyphs.begin(), glyphs.end());
    }
    return out;
  }

 private:
  std::vector<RunGlyph> boxes(const std::u32string& codes, const RunItem& item, int pixels) {
    std::vector<RunGlyph> out;
    const float advance = tofu(pixels, chain_[0]->metrics(pixels).ascent).advance;
    for (std::size_t i = item.begin; i < item.end; ++i) {
      if (is_invisible(codes[i])) continue;
      missing_.report(codes[i]);
      out.push_back({core::kTofuKey, advance, 0.0f, 0.0f, i});
    }
    return out;
  }
  const Chain& chain_;
  MissingGlyphs& missing_;
};

class TrueType final : public core::FontSource {
 public:
  TrueType(Chain chain, bool crisp)
      : chain_(std::move(chain)), crisp_(crisp), missing_("font \"" + chain_[0]->file().generic_string() + "\"") {}
  int native() const override { return 0; }
  const Chain& chain() const { return chain_; }

  core::ShapedText shape(const core::TextRequest& r) const override {
    const int pixels = std::clamp(r.pixels, 1, core::kMaxTextPixels);
    const std::vector<core::ShapedLine>& lines = shaped(r, pixels);
    bool dropped = false;
    core::FontAtlasPtr atlas = atlas_for(pixels).ensure(
        keys_of(lines), [&](core::GlyphKey k) { return render(k, pixels); }, dropped);
    if (dropped) missing_.report_full(pixels);
    return {std::move(atlas), lines};
  }

 private:
  const std::vector<core::ShapedLine>& shaped(const core::TextRequest& r, int pixels) const {
    const Key key{std::string(r.text), pixels, r.dir, r.wrap};
    if (const auto it = shapes_.find(key); it != shapes_.end()) return it->second;
    if (shapes_.size() >= kShapes) shapes_.clear();
    ChainShaper shaper(chain_, missing_);
    return shapes_[key] = shape_lines(shaper, core::decode_utf8(r.text), pixels, r.dir, r.wrap);
  }

  GlyphAtlas& atlas_for(int pixels) const {
    for (auto it = atlases_.begin(); it != atlases_.end(); ++it) {
      if (it->first != pixels) continue;
      atlases_.splice(atlases_.begin(), atlases_, it);
      return atlases_.front().second;
    }
    if (atlases_.size() >= core::kGlyphAtlasSizes) atlases_.pop_back();
    atlases_.emplace_front(pixels, GlyphAtlas(chain_[0]->metrics(pixels), crisp_ ? core::Filter::nearest : core::Filter::linear));
    return atlases_.front().second;
  }

  GlyphBitmap render(core::GlyphKey key, int pixels) const {
    const float ascent = chain_[0]->metrics(pixels).ascent;
    if (key == core::kTofuKey) return tofu(pixels, ascent);
    const auto font = static_cast<std::size_t>(key >> 32);
    return chain_[font]->render(static_cast<std::uint32_t>(key & 0xFFFFFFFFu), pixels, ascent, crisp_);
  }

  using Key = std::tuple<std::string, int, core::TextDir, float>;
  static constexpr std::size_t kShapes = 512;  // shaped texts kept (score labels change every frame)
  Chain chain_;
  bool crisp_;
  mutable MissingGlyphs missing_;
  mutable std::map<Key, std::vector<core::ShapedLine>> shapes_;
  mutable std::list<std::pair<int, GlyphAtlas>> atlases_;  // most recent first
};

}  // namespace

core::FontSourcePtr load_truetype(const std::filesystem::path& file, bool crisp,
                                  const std::vector<core::FontSourcePtr>& fallbacks) {
  if (fallbacks.size() > core::kMaxFontFallbacks) {
    throw core::Error("font \"" + file.generic_string() + "\" lists " + std::to_string(fallbacks.size()) +
                      " fallbacks; at most " + std::to_string(core::kMaxFontFallbacks));
  }
  Chain chain{std::make_shared<const FontFile>(read_font(file), file)};
  for (const core::FontSourcePtr& f : fallbacks) {
    const auto* tt = dynamic_cast<const TrueType*>(f.get());
    if (!tt) throw core::Error("font \"" + file.generic_string() + "\": fallbacks must be .ttf/.otf fonts, not BMFont or the built-in font");
    chain.push_back(tt->chain().front());
  }
  return std::make_shared<TrueType>(std::move(chain), crisp);
}

}  // namespace alpha::platform
