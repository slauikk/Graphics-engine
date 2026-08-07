#include "editor_camera.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <glm/gtc/constants.hpp>

namespace core {
namespace {

bool isFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

bool isValid(const geometry::AxisAlignedBounds& bounds) {
    return bounds.valid && isFinite(bounds.minimum) && isFinite(bounds.maximum) &&
           bounds.minimum.x <= bounds.maximum.x &&
           bounds.minimum.y <= bounds.maximum.y &&
           bounds.minimum.z <= bounds.maximum.z;
}

} // namespace

std::optional<glm::vec3> calculateFramedCameraPosition(
    const geometry::AxisAlignedBounds& worldBounds,
    const glm::vec3& viewDirection,
    float verticalFovDegrees,
    float aspectRatio,
    float nearPlane,
    float farPlane,
    float margin) {
    const double directionLength = glm::length(glm::dvec3(viewDirection));
    if (!isValid(worldBounds) || !isFinite(viewDirection) ||
        !std::isfinite(directionLength) || directionLength <= 0.0f ||
        !std::isfinite(verticalFovDegrees) || verticalFovDegrees <= 0.0f ||
        verticalFovDegrees >= 179.0f || !std::isfinite(aspectRatio) ||
        aspectRatio <= 0.0f || !std::isfinite(nearPlane) || nearPlane <= 0.0f ||
        !std::isfinite(farPlane) || farPlane <= nearPlane ||
        !std::isfinite(margin) || margin < 1.0f) {
        return std::nullopt;
    }

    const glm::dvec3 minimum(worldBounds.minimum);
    const glm::dvec3 maximum(worldBounds.maximum);
    const glm::dvec3 center = minimum + (maximum - minimum) * 0.5;
    const double verticalHalfFov =
        static_cast<double>(verticalFovDegrees) * glm::pi<double>() / 360.0;
    const double verticalTangent = std::tan(verticalHalfFov);
    const double horizontalTangent =
        verticalTangent * static_cast<double>(aspectRatio);
    if (!std::isfinite(verticalTangent) || verticalTangent <= 0.0 ||
        !std::isfinite(horizontalTangent) || horizontalTangent <= 0.0) {
        return std::nullopt;
    }

    const glm::dvec3 direction = glm::dvec3(viewDirection) / directionLength;
    glm::dvec3 right = glm::cross(direction, glm::dvec3(0.0, 1.0, 0.0));
    if (glm::dot(right, right) <= 1.0e-12) {
        right = glm::cross(direction, glm::dvec3(0.0, 0.0, 1.0));
    }
    right = glm::normalize(right);
    const glm::dvec3 up = glm::normalize(glm::cross(right, direction));

    const std::array<glm::dvec3, 8> corners = {
        glm::dvec3(minimum.x, minimum.y, minimum.z),
        glm::dvec3(maximum.x, minimum.y, minimum.z),
        glm::dvec3(minimum.x, maximum.y, minimum.z),
        glm::dvec3(maximum.x, maximum.y, minimum.z),
        glm::dvec3(minimum.x, minimum.y, maximum.z),
        glm::dvec3(maximum.x, minimum.y, maximum.z),
        glm::dvec3(minimum.x, maximum.y, maximum.z),
        glm::dvec3(maximum.x, maximum.y, maximum.z)};

    const double nearClearance = static_cast<double>(nearPlane) * 1.05;
    double distance = static_cast<double>(nearPlane) * 4.0;
    double farthestDepthOffset = -std::numeric_limits<double>::infinity();
    for (const glm::dvec3& corner : corners) {
        const glm::dvec3 offset = corner - center;
        const double depthOffset = glm::dot(offset, direction);
        const double horizontalOffset = std::abs(glm::dot(offset, right));
        const double verticalOffset = std::abs(glm::dot(offset, up));
        distance = std::max({
            distance,
            nearClearance - depthOffset,
            horizontalOffset * static_cast<double>(margin) / horizontalTangent -
                depthOffset,
            verticalOffset * static_cast<double>(margin) / verticalTangent -
                depthOffset});
        farthestDepthOffset = std::max(farthestDepthOffset, depthOffset);
    }

    if (!std::isfinite(distance) || !std::isfinite(farthestDepthOffset) ||
        distance + farthestDepthOffset > static_cast<double>(farPlane) * 0.95) {
        return std::nullopt;
    }

    const glm::dvec3 framedPosition = center - direction * distance;
    const double maximumFloat =
        static_cast<double>((std::numeric_limits<float>::max)());
    if (!std::isfinite(framedPosition.x) || !std::isfinite(framedPosition.y) ||
        !std::isfinite(framedPosition.z) || std::abs(framedPosition.x) > maximumFloat ||
        std::abs(framedPosition.y) > maximumFloat ||
        std::abs(framedPosition.z) > maximumFloat) {
        return std::nullopt;
    }

    return glm::vec3(framedPosition);
}

} // namespace core
