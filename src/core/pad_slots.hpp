#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "core/input.hpp"

namespace alpha::core {

// Stable player slots for gamepads, in connection order. A slot freed by a
// disconnect stays reserved for that device kind (`key`, e.g. its GUID): when
// a device with the same key reconnects it gets the slot back. New devices
// take the first never-used slot, then the first free reserved one.
class PadSlots {
 public:
  // Slot for device `id` (nullopt when all slots are taken). Idempotent.
  std::optional<std::size_t> connect(std::uint32_t id, std::string_view key);
  // The slot device `id` held, now free; nullopt if it had none.
  std::optional<std::size_t> disconnect(std::uint32_t id);
  std::optional<std::size_t> slot_of(std::uint32_t id) const;
  // Players with a device: 1 (keyboard) up to the highest connected slot + 1.
  std::size_t players() const;

 private:
  struct Slot {
    std::uint32_t id = 0;
    std::string key;
    bool connected = false;
    bool used = false;
  };
  std::optional<std::size_t> free_slot(std::string_view key) const;

  std::array<Slot, kMaxPlayers> slots_{};
};

}  // namespace alpha::core
