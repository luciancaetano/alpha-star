#include "core/pad_slots.hpp"

namespace alpha::core {

// Preference: this device kind's reserved slot, a never-used slot, any free slot.
std::optional<std::size_t> PadSlots::free_slot(std::string_view key) const {
  const auto first = [this](auto pred) -> std::optional<std::size_t> {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
      if (pred(slots_[i])) return i;
    }
    return std::nullopt;
  };
  if (const auto mine = first([key](const Slot& s) { return !s.connected && s.used && s.key == key; })) return mine;
  if (const auto unused = first([](const Slot& s) { return !s.used; })) return unused;
  return first([](const Slot& s) { return !s.connected; });
}

std::optional<std::size_t> PadSlots::connect(std::uint32_t id, std::string_view key) {
  if (const auto existing = slot_of(id)) return existing;
  const auto slot = free_slot(key);
  if (slot) slots_[*slot] = Slot{id, std::string(key), true, true};
  return slot;
}

std::optional<std::size_t> PadSlots::disconnect(std::uint32_t id) {
  const auto slot = slot_of(id);
  if (slot) slots_[*slot].connected = false;
  return slot;
}

std::optional<std::size_t> PadSlots::slot_of(std::uint32_t id) const {
  for (std::size_t i = 0; i < slots_.size(); ++i) {
    if (slots_[i].connected && slots_[i].id == id) return i;
  }
  return std::nullopt;
}

std::size_t PadSlots::players() const {
  std::size_t n = 1;
  for (std::size_t i = 0; i < slots_.size(); ++i) {
    if (slots_[i].connected) n = i + 1;
  }
  return n;
}

}  // namespace alpha::core
