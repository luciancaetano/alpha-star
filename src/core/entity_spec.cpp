#include "core/entity_spec.hpp"

#include <utility>

namespace alpha::core {

ShapeSpec::ShapeSpec(Geometry g, Color f, std::optional<Color> o, ImagePtr img)
    : geometry(g), fill(f), outline(o), image(std::move(img)) {}

}  // namespace alpha::core
