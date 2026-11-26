#pragma once

#include <quickjs.h>

#include "net/message.hpp"

namespace alpha::script {

// alpha:net natives (netOpen/netSend/netReceive/netInfo/netClose and the
// per-step netDeliver/netFlush) on the active instance's net::Hub; with
// ALPHA_DEBUG_TOOLS also the alpha/test `netFake` natives.
void install_net_bindings(JSContext* ctx, JSValueConst target);

// A script's message data: a string (text) or a Uint8Array (bytes).
net::Message message_arg(JSContext* ctx, JSValueConst v, const char* what);
// The value scripts receive: a string when text, else a Uint8Array.
JSValue new_message_data(JSContext* ctx, const net::Message& m);

}  // namespace alpha::script
