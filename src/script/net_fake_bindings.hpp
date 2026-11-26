#pragma once

#include <quickjs.h>

namespace alpha::script {

// alpha/test `netFake` natives (ALPHA_DEBUG_TOOLS only): netFakeServer,
// netFakePoll, netFakePeer, netFakeGeneration over the Vm's net::FakeNet.
void install_net_fake_bindings(JSContext* ctx, JSValueConst target);

}  // namespace alpha::script
