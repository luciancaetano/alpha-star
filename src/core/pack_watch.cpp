#include "core/pack_watch.hpp"

#include <system_error>

namespace alpha::core {

namespace fs = std::filesystem;

PackWatch::PackWatch(fs::path dir) : dir_(std::move(dir)), seen_(scan()) {}

bool PackWatch::poll(double now) {
  if (now - last_scan_ < kInterval) return false;
  last_scan_ = now;
  Snapshot current = scan();
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

// Never throws: a file vanishing mid-scan (an export rewriting it) just
// changes the snapshot, and the next quiet scan settles it.
PackWatch::Snapshot PackWatch::scan() const {
  Snapshot out;
  std::error_code ec;
  for (fs::recursive_directory_iterator it(dir_, ec), end; !ec && it != end; it.increment(ec)) {
    std::error_code file_ec;
    if (!it->is_regular_file(file_ec)) continue;
    const auto size = it->file_size(file_ec);
    const auto time = it->last_write_time(file_ec);
    if (file_ec) continue;
    out[fs::relative(it->path(), dir_, file_ec).generic_string()] = {size, static_cast<std::int64_t>(time.time_since_epoch().count())};
  }
  return out;
}

}  // namespace alpha::core
