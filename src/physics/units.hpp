#pragma once

#include <box2d/box2d.h>

#include <numbers>

#include "core/vec2.hpp"

// Engine units (pixels, degrees) <-> Box2D units (meters, radians).
namespace alpha::physics {

// Box2D is tuned for 0.1..10 m objects; game objects are ~5..500 px.
constexpr float kPixelsPerMeter = 50.0f;
constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

constexpr b2Vec2 to_meters(core::Vec2 v) { return {v.x / kPixelsPerMeter, v.y / kPixelsPerMeter}; }
constexpr core::Vec2 to_pixels(b2Vec2 v) { return {v.x * kPixelsPerMeter, v.y * kPixelsPerMeter}; }

}  // namespace alpha::physics
