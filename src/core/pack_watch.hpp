#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>

namespace alpha::core {

// Hot reload's file watch (dev/release builds only): polls the modification
// times of every file under a pack directory from the main loop. poll() says
// "reload" once per burst of changes, when the pack has been quiet for
// `quiet` seconds, so an export writing several files reloads once.
class PackWatch {
 public:
  static constexpr double kInterval = 0.25;  // seconds between scans (~4 per second)
  static constexpr double kQuiet = 0.3;      // seconds without changes before a reload

  explicit PackWatch(std::filesystem::path dir);
  // now: seconds on any monotonic clock. True when the game should reload.
  bool poll(double now);

 private:
  // relative path -> (size, modification time ticks)
  using Snapshot = std::map<std::string, std::pair<std::uintmax_t, std::int64_t>>;
  Snapshot scan() const;

  std::filesystem::path dir_;
  Snapshot seen_;
  double last_scan_ = -1e9;
  double changed_at_ = 0.0;
  bool pending_ = false;
};

}  // namespace alpha::core
