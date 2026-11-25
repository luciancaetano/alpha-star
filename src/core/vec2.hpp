#pragma once

namespace alpha::core {

// 2D vector in pixels (or pixels/s). Origin top-left, +y down.
struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
  bool operator==(const Vec2&) const = default;
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }

}  // namespace alpha::core
