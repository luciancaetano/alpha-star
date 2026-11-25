#pragma once

#include <quickjs.h>

#include <string>

#include "core/mixer.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// ctx.audio natives: sounds with pan, volume buses, streamed music.
void install_audio_bindings(JSContext* ctx, JSValueConst target);

// A bus name -> its id; unknown names throw listing the buses.
core::BusId bus_arg(JSContext* ctx, ScriptState& s, JSValueConst v, const std::string& what);

// Samples of a synth spec (assets.sounds entry or audio.tone).
core::SoundPtr synth_samples(JSContext* ctx, JSValueConst spec, const std::string& what);

}  // namespace alpha::script
