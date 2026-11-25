#include "core/pack_watch.hpp"

namespace alpha::core {

PackWatch::PackWatch(Scan scan) : scan_(std::move(scan)), seen_(scan_()) {}

bool PackWatch::poll(double now) {
  if (now - last_scan_ < kInterval) return false;
  last_scan_ = now;
  Snapshot current = scan_();
  if (current != seen_) {
    seen_ = std::move(current);
    changed_at_ = now;
    pending_ = true;
    return false;
  }
  if (!pending_ || now - changed_at_ < kQuiet) return false;
  pending_ = false;
  return true;
}

}  // namespace alpha::core
