#include "core/entity_spec.hpp"

#include <utility>

namespace alpha::core {

ShapeSpec::ShapeSpec(Geometry g, Color f, std::optional<Color> o, ImagePtr img)
    : geometry(g), fill(f), outline(o), image(std::move(img)) {}

ColliderSpec::ColliderSpec(Geometry g, bool is_sensor) : geometry(g), sensor(is_sensor) {}

BodySpec::BodySpec(BodyType t, ColliderSpec c, Vec2 v) : type(t), collider(c), velocity(v) {}

DrawItem::DrawItem(ShapeSpec s, Vec2 p, float degrees) : shape(std::move(s)), pos(p), rotation(degrees) {}

}  // namespace alpha::core
