#pragma once

#include <quickjs.h>

#include <string>
#include <vector>

#include "core/synth.hpp"

namespace alpha::script {

// Parses a synth spec: one note object or an array of notes played in order.
// Note: { wave, freq: hz | [from, to], duration, volume?, attack?, release? }.
// Throws core::Error prefixed with `what` and naming the bad field.
std::vector<core::Tone> parse_synth(JSContext* ctx, JSValueConst spec, const std::string& what);

}  // namespace alpha::script
