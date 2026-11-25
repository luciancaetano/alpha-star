#include "core/entity_spec.hpp"

#include <utility>

namespace alpha::core {

ShapeSpec::ShapeSpec(Geometry g, Color f, std::optional<Color> o, ImagePtr img)
    : geometry(g), fill(f), outline(o), image(std::move(img)) {}

DrawItem::DrawItem(ShapeSpec s, Vec2 p, float degrees) : shape(std::move(s)), pos(p), rotation(degrees) {}

}  // namespace alpha::core
