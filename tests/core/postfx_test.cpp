#include <doctest/doctest.h>

#include <string>

#include "core/error.hpp"
#include "core/lighting.hpp"
#include "core/postfx.hpp"

using namespace alpha::core;

namespace {

std::string error_of(const std::function<void()>& fn) {
  try {
    fn();
  } catch (const Error& e) {
    return e.what();
  }
  return "";
}

}  // namespace

TEST_CASE("make_effect fills defaults and takes known params") {
  const Effect crt = make_effect("crt", {});
  CHECK(crt.kind == EffectKind::Crt);
  CHECK(crt.strength == doctest::Approx(0.5));
  const Effect v = make_effect("vignette", {{"strength", 0.8f}, {"radius", 0.2f}});
  CHECK(v.strength == doctest::Approx(0.8));
  CHECK(v.radius == doctest::Approx(0.2));
  CHECK(make_effect("pixelate", {{"size", 3.4f}}).size == 3.0f);
  CHECK(make_effect("tint", {}, Color{255, 0, 0, 255}).color == Color{255, 0, 0, 255});
}

TEST_CASE("make_effect errors name the effect, the param and what is allowed") {
  CHECK(error_of([] { make_effect("blur", {}); }) == "postfx: unknown effect \"blur\"; have: bloom crt pixelate tint vignette");
  CHECK(error_of([] { make_effect("crt", {{"size", 2.0f}}); }) == "postfx \"crt\": unknown param \"size\"; have: strength");
  CHECK(error_of([] { make_effect("crt", {}, Color{}); }).find("unknown param \"color\"") != std::string::npos);
  const std::string range = error_of([] { make_effect("bloom", {{"strength", 3.0f}}); });
  CHECK(range.find("postfx \"bloom\".strength must be 0") == 0);
  CHECK(range.find("got 3") != std::string::npos);
}

TEST_CASE("PostFx keeps first-add order and replaces params in place") {
  PostFx fx;
  fx.put(make_effect("bloom", {}));
  fx.put(make_effect("crt", {}));
  fx.put(make_effect("bloom", {{"strength", 1.5f}}));
  REQUIRE(fx.chain().size() == 2);
  CHECK(fx.chain()[0].kind == EffectKind::Bloom);
  CHECK(fx.chain()[0].strength == doctest::Approx(1.5));
  CHECK(fx.remove(EffectKind::Bloom));
  CHECK_FALSE(fx.remove(EffectKind::Bloom));
  CHECK(fx.chain().size() == 1);
  CHECK(effect_name(fx.chain()[0].kind) == "crt");
  CHECK(effect_kind("pixelate") == EffectKind::Pixelate);
  CHECK_FALSE(effect_kind("nope").has_value());
}

TEST_CASE("needs_scene only for effects that read the frame") {
  CHECK_FALSE(needs_scene({make_effect("crt", {}), make_effect("tint", {})}));
  CHECK(needs_scene({make_effect("vignette", {}), make_effect("pixelate", {})}));
  CHECK(needs_scene({make_effect("bloom", {})}));
}

TEST_CASE("make_light validates radius and intensity") {
  CHECK(make_light(50, {}, 2).intensity == 2.0f);
  CHECK(error_of([] { make_light(0, {}, 1); }).find("spawn: light.radius must be 1..4096") == 0);
  CHECK(error_of([] { make_light(10, {}, 9); }).find("spawn: light.intensity must be 0..8") == 0);
}

TEST_CASE("light_discs place lights through the camera and drop off-screen ones") {
  const View view{{50, 50}, 2.0f, 0.0f, {100, 100}};
  std::vector<LightItem> lights{
      {{50, 50}, {10, {100, 200, 255, 255}, 2.0f}},
      {{60, 50}, {10, {255, 255, 255, 255}, 1.0f}, 0.0f},  // fixed to the screen
      {{500, 500}, {10, {255, 255, 255, 255}, 1.0f}},
  };
  const auto discs = light_discs(lights, view);
  REQUIRE(discs.size() == 2);
  CHECK(discs[0].center.x == doctest::Approx(50));
  CHECK(discs[0].radius == doctest::Approx(20));
  CHECK(discs[0].color == Color{200, 255, 255, 255});
  CHECK(discs[1].center.x == doctest::Approx(60));
  CHECK(discs[1].radius == doctest::Approx(10));
}
