#include "script/test_report.hpp"

#include <cstdio>

#include "core/version.hpp"
#include "script/bytecode.hpp"
#include "script/host.hpp"

namespace alpha::script {

namespace {

// Error text indented under its FAIL line, without trailing blank lines.
std::string indented(const std::string& text) {
  std::string out;
  for (const char c : text.substr(0, text.find_last_not_of("\r\n") + 1)) {
    out += c == '\n' ? std::string("\n    ") : std::string(1, c);
  }
  return out;
}

}  // namespace

int run_test_file(const std::string& path, const std::string& asset_dir) {
  Host host([](const std::string&) { return std::string(); });
  const auto results = host.test(path, asset_dir);
  std::size_t failed = 0;
  for (const auto& r : results) {
    failed += r.passed() ? 0U : 1U;
    if (r.passed()) std::printf("PASS %s\n", r.name.c_str());
    else std::printf("FAIL %s\n    %s\n", r.name.c_str(), indented(r.error).c_str());
  }
  if (results.empty()) std::printf("no tests: %s exports no functions\n", path.c_str());
  std::printf("%zu passed, %zu failed\n", results.size() - failed, failed);
  return failed == 0 && !results.empty() ? 0 : 1;
}

int run_command(const core::CliOptions& o) {
  if (o.version) return std::printf("alpha-star %s\n", core::version().data()) < 0 ? 1 : 0;
  if (!o.test_file.empty()) return run_test_file(o.test_file, o.test_assets);
  if (!o.compile_source.empty()) return compile_file(o.compile_source, o.compile_out);
  return -1;
}

}  // namespace alpha::script
