#pragma once

#include <cstddef>

#include "core/log_ring.hpp"
#include "mcp/dispatch.hpp"
#include "mcp/session.hpp"

namespace alpha::mcp {

// Response fields of `state`: scene, paused, error (null if none), frame, fps,
// entities [{id, tags, pos, rotation, z, velocity}], ui {doc: {shown, texts}}.
Fields state_fields(const GameState& s);
// Response fields of `logs`: lines [{n, level, text}] numbered since start,
// those with n >= since that are still kept; next = cursor for the next call;
// dropped = requested lines already gone from the ring.
Fields log_fields(const core::LogRing& log, std::size_t since);

}  // namespace alpha::mcp
