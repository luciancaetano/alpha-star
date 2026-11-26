#include "script/net_fake_bindings.hpp"

#include <array>

#include "core/error.hpp"
#include "net/fake_net.hpp"
#include "net/ws_handshake.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/net_bindings.hpp"

namespace alpha::script {

namespace {

std::uint32_t id_arg(JSContext* ctx, JSValueConst v, const char* what) { return static_cast<std::uint32_t>(number_arg(ctx, v, what)); }

// netFakeServer(kind, endpoint, latencySteps, loss) -> server id.
JSValue fake_server(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  const std::string kind = string_arg(ctx, argv[0], "kind");
  const net::Kind k = kind == "ws" ? net::Kind::ws : kind == "udp" ? net::Kind::udp : net::Kind::tcp;
  net::FakeNet::Options o;
  o.latency_steps = static_cast<int>(number_arg(ctx, argv[2], "latencySteps"));
  o.loss = number_arg(ctx, argv[3], "loss");
  std::string endpoint = string_arg(ctx, argv[1], "endpoint");
  if (k == net::Kind::ws) endpoint = net::endpoint_of(net::parse_ws_url(endpoint));  // "ws://h/" names ws://h:80/
  return JS_NewUint32(ctx, instances_of(ctx).fake_net().server(k, endpoint, o));
}

JSValue ids_array(JSContext* ctx, const std::vector<std::uint32_t>& ids) { return new_ids(ctx, ids); }

// netFakePoll(server) -> { connects: [peer], messages: [[peer, data]], closes: [peer] }.
JSValue fake_poll(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  const net::FakeNet::Events ev = instances_of(ctx).fake_net().poll(id_arg(ctx, argv[0], "server"));
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "connects", ids_array(ctx, ev.connects));
  JS_SetPropertyStr(ctx, o, "closes", ids_array(ctx, ev.closes));
  JSValue messages = JS_NewArray(ctx);
  for (std::uint32_t i = 0; i < ev.messages.size(); ++i) {
    JSValue pair = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, pair, 0, JS_NewUint32(ctx, ev.messages[i].first));
    JS_SetPropertyUint32(ctx, pair, 1, new_message_data(ctx, ev.messages[i].second));
    JS_SetPropertyUint32(ctx, messages, i, pair);
  }
  JS_SetPropertyStr(ctx, o, "messages", messages);
  return o;
}

// netFakePeer(peer, "send", data) -> bool | netFakePeer(peer, "close", code).
JSValue fake_peer(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  net::FakeNet& fake = instances_of(ctx).fake_net();
  const std::uint32_t peer = id_arg(ctx, argv[0], "peer");
  if (string_arg(ctx, argv[1], "peer op") == "send") return JS_NewBool(ctx, fake.send(peer, message_arg(ctx, argv[2], "peer.send")));
  fake.close(peer, static_cast<int>(number_arg(ctx, argv[2], "close code")));
  return JS_UNDEFINED;
}

JSValue fake_generation(JSContext* ctx, ScriptState&, JSValueConst*) {
  return JS_NewUint32(ctx, instances_of(ctx).fake_generation());
}

constexpr std::array<Native, 4> kNatives{{
    {"netFakeGeneration", guarded<fake_generation>, 0},
    {"netFakeServer", guarded<fake_server>, 4},
    {"netFakePoll", guarded<fake_poll>, 1},
    {"netFakePeer", guarded<fake_peer>, 3},
}};

}  // namespace

void install_net_fake_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
