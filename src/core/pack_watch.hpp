#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>

namespace alpha::core {

// Hot reload's file watch (dev/release builds only): polls a snapshot of the
// pack's files from the main loop. poll() says "reload" once per burst of
// changes, when the pack has been quiet for `kQuiet` seconds, so an export
// writing several files reloads once. Pure: the snapshot comes from `scan`
// (the platform's file layer lists the pack directory).
class PackWatch {
 public:
  static constexpr double kInterval = 0.25;  // seconds between scans (~4 per second)
  static constexpr double kQuiet = 0.3;      // seconds without changes before a reload

  // relative path -> (size, modification time ticks)
  using Snapshot = std::map<std::string, std::pair<std::uint64_t, std::int64_t>>;
  using Scan = std::function<Snapshot()>;

  explicit PackWatch(Scan scan);
  // now: seconds on any monotonic clock. True when the game should reload.
  bool poll(double now);

 private:
  Scan scan_;
  Snapshot seen_;
  double last_scan_ = -1e9;
  double changed_at_ = 0.0;
  bool pending_ = false;
};

}  // namespace alpha::core
