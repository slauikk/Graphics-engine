#ifndef CORE_EDITOR_PLACEMENT_H
#define CORE_EDITOR_PLACEMENT_H

#include <optional>
#include <span>

#include <glm/glm.hpp>

#include "math/geometry.h"

namespace core {

std::optional<glm::vec3> findNearestFreeCubeGridPosition(
    const glm::vec3& basePosition,
    std::span<const geometry::AxisAlignedBounds> occupiedBounds,
    float maximumCenterCoordinate,
    int maximumRing = 64,
    float padding = 0.1f);

} // namespace core

#endif // CORE_EDITOR_PLACEMENT_H
