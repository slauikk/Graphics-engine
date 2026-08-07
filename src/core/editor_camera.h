#ifndef CORE_EDITOR_CAMERA_H
#define CORE_EDITOR_CAMERA_H

#include <optional>

#include <glm/glm.hpp>

#include "math/geometry.h"

namespace core {

std::optional<glm::vec3> calculateFramedCameraPosition(
    const geometry::AxisAlignedBounds& worldBounds,
    const glm::vec3& viewDirection,
    float verticalFovDegrees,
    float aspectRatio,
    float nearPlane,
    float farPlane,
    float margin = 1.15f);

} // namespace core

#endif // CORE_EDITOR_CAMERA_H
