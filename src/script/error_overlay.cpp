#include "script/error_overlay.hpp"

#include <sstream>

namespace alpha::script {

namespace {

constexpr float kTextSize = 8.0f;
constexpr float kMargin = 4.0f;
const core::Color kErrorColor{255, 90, 90, 255};

void wrap_line(const std::string& line, std::size_t columns, std::vector<std::string>& out) {
  std::size_t start = 0;
  while (line.size() - start > columns) {
    const std::size_t space = line.rfind(' ', start + columns);
    const bool usable = space != std::string::npos && space > start;
    const std::size_t end = usable ? space : start + columns;
    out.push_back(line.substr(start, end - start));
    start = usable ? end + 1 : end;
  }
  out.push_back(line.substr(start));
}

}  // namespace

std::vector<std::string> wrap(const std::string& text, std::size_t columns) {
  std::vector<std::string> out;
  std::istringstream lines(text);
  for (std::string line; std::getline(lines, line);) wrap_line(line, columns, out);
  return out;
}

std::vector<core::TextItem> error_overlay(const std::string& error, float width) {
  const auto columns = static_cast<std::size_t>((width - 2 * kMargin) / kTextSize);
  std::vector<core::TextItem> items;
  float y = kMargin + kTextSize / 2;
  for (const std::string& line : wrap("SCRIPT ERROR (game paused)\n" + error, columns)) {
    items.push_back({line, {kMargin, y}, kTextSize, kErrorColor, core::Align::Left});
    y += kTextSize + 2;
  }
  return items;
}

}  // namespace alpha::script
