#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Splits a bidi run into items shaped separately: one font of the chain and
// one script each (HarfBuzz's Unicode data). Marks, joiners, variation
// selectors and script-neutral characters (spaces, digits, punctuation) stay
// with the item before them when its font has them.
namespace alpha::platform {

inline constexpr std::uint32_t kNoFont = 0xFFFFFFFFu;  // no font has it: drawn as a box

struct RunItem {
  std::size_t begin = 0;
  std::size_t end = 0;
  std::uint32_t font = 0;
};

// covers(font, c): whether font `font` (0 .. fonts - 1) of the chain has c.
using Covers = std::function<bool(std::uint32_t font, char32_t c)>;
std::vector<RunItem> run_items(const std::u32string& codes, std::size_t begin, std::size_t end, std::uint32_t fonts,
                               const Covers& covers);

// Control and format characters (tab, ZWJ, ...): never drawn as a box.
bool is_invisible(char32_t c);

}  // namespace alpha::platform
