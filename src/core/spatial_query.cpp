#include "spatial_query.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/gtc/matrix_inverse.hpp>

namespace core {
namespace {

bool isFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

bool isFinite(const glm::mat4& value) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (!std::isfinite(value[column][row])) {
                return false;
            }
        }
    }
    return true;
}

bool isValid(const geometry::AxisAlignedBounds& bounds) {
    return bounds.valid && isFinite(bounds.minimum) && isFinite(bounds.maximum) &&
           bounds.minimum.x <= bounds.maximum.x &&
           bounds.minimum.y <= bounds.maximum.y &&
           bounds.minimum.z <= bounds.maximum.z;
}

bool isAffine(const glm::mat4& value) {
    return value[0][3] == 0.0f && value[1][3] == 0.0f &&
           value[2][3] == 0.0f && value[3][3] == 1.0f;
}

struct LocalRay {
    glm::vec3 origin;
    glm::vec3 direction;
};

std::optional<LocalRay> transformRay(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const glm::mat4& localToWorld) {
    if (!isFinite(localToWorld) || !isAffine(localToWorld)) {
        return std::nullopt;
    }

    const glm::mat4 worldToLocal = glm::inverse(localToWorld);
    if (!isFinite(worldToLocal)) {
        return std::nullopt;
    }

    LocalRay ray;
    ray.origin = glm::vec3(worldToLocal * glm::vec4(origin, 1.0f));
    ray.direction = glm::vec3(worldToLocal * glm::vec4(direction, 0.0f));
    if (!isFinite(ray.origin) || !isFinite(ray.direction)) {
        return std::nullopt;
    }
    return ray;
}

} // namespace

std::optional<geometry::AxisAlignedBounds> calculateIndexedBounds(
    std::span<const float> interleavedVertices,
    std::size_t componentsPerVertex,
    std::span<const std::uint32_t> indices) {
    if (componentsPerVertex < 3 || interleavedVertices.empty() || indices.empty() ||
        interleavedVertices.size() % componentsPerVertex != 0) {
        return std::nullopt;
    }

    const std::size_t vertexCount = interleavedVertices.size() / componentsPerVertex;
    geometry::AxisAlignedBounds bounds;
    bounds.minimum = glm::vec3((std::numeric_limits<float>::max)());
    bounds.maximum = glm::vec3((std::numeric_limits<float>::lowest)());

    for (std::uint32_t index : indices) {
        const std::size_t vertex = static_cast<std::size_t>(index);
        if (vertex >= vertexCount) {
            return std::nullopt;
        }
        for (glm::length_t axis = 0; axis < 3; ++axis) {
            const float value = interleavedVertices[
                vertex * componentsPerVertex + static_cast<std::size_t>(axis)];
            if (!std::isfinite(value)) {
                return std::nullopt;
            }
            bounds.minimum[axis] = std::min(bounds.minimum[axis], value);
            bounds.maximum[axis] = std::max(bounds.maximum[axis], value);
        }
    }

    bounds.valid = true;
    return bounds;
}

std::optional<float> intersectRayAabb(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const geometry::AxisAlignedBounds& bounds) {
    if (!isFinite(origin) || !isFinite(direction) || !isValid(bounds)) {
        return std::nullopt;
    }

    if (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f) {
        return std::nullopt;
    }

    double nearest = 0.0;
    double farthest = (std::numeric_limits<double>::infinity)();
    for (int axis = 0; axis < 3; ++axis) {
        const double axisDirection = static_cast<double>(direction[axis]);
        const double axisOrigin = static_cast<double>(origin[axis]);
        const double minimum = static_cast<double>(bounds.minimum[axis]);
        const double maximum = static_cast<double>(bounds.maximum[axis]);

        if (axisDirection == 0.0) {
            if (axisOrigin < minimum || axisOrigin > maximum) {
                return std::nullopt;
            }
            continue;
        }

        double first = (minimum - axisOrigin) / axisDirection;
        double second = (maximum - axisOrigin) / axisDirection;
        if (first > second) {
            std::swap(first, second);
        }

        nearest = std::max(nearest, first);
        farthest = std::min(farthest, second);
        if (farthest < nearest) {
            return std::nullopt;
        }
    }

    if (!std::isfinite(nearest) ||
        nearest > static_cast<double>((std::numeric_limits<float>::max)())) {
        return std::nullopt;
    }
    return static_cast<float>(nearest);
}

std::optional<float> intersectRayTransformedAabb(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const geometry::AxisAlignedBounds& localBounds,
    const glm::mat4& localToWorld) {
    if (!isValid(localBounds)) {
        return std::nullopt;
    }

    const std::optional<LocalRay> ray = transformRay(origin, direction, localToWorld);
    if (!ray.has_value()) {
        return std::nullopt;
    }

    return intersectRayAabb(ray->origin, ray->direction, localBounds);
}

std::optional<float> intersectRayIndexedTriangles(
    const glm::vec3& origin,
    const glm::vec3& direction,
    std::span<const glm::vec3> positions,
    std::span<const std::uint32_t> indices,
    float minimumDistance,
    float maximumDistance) {
    if (!isFinite(origin) || !isFinite(direction) || positions.empty() ||
        indices.size() < 3 || indices.size() % 3 != 0 ||
        !std::isfinite(minimumDistance) || !std::isfinite(maximumDistance) ||
        minimumDistance < 0.0f || maximumDistance < minimumDistance ||
        (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f)) {
        return std::nullopt;
    }

    constexpr double barycentricTolerance = 1.0e-9;
    double nearest = static_cast<double>(maximumDistance);
    bool found = false;
    for (std::size_t triangle = 0; triangle < indices.size(); triangle += 3) {
        const std::size_t firstIndex = static_cast<std::size_t>(indices[triangle]);
        const std::size_t secondIndex = static_cast<std::size_t>(indices[triangle + 1]);
        const std::size_t thirdIndex = static_cast<std::size_t>(indices[triangle + 2]);
        if (firstIndex >= positions.size() || secondIndex >= positions.size() ||
            thirdIndex >= positions.size()) {
            return std::nullopt;
        }

        const glm::vec3& firstPosition = positions[firstIndex];
        const glm::vec3& secondPosition = positions[secondIndex];
        const glm::vec3& thirdPosition = positions[thirdIndex];
        if (!isFinite(firstPosition) || !isFinite(secondPosition) ||
            !isFinite(thirdPosition)) {
            return std::nullopt;
        }

        const glm::dvec3 first(firstPosition);
        const glm::dvec3 edgeOne = glm::dvec3(secondPosition) - first;
        const glm::dvec3 edgeTwo = glm::dvec3(thirdPosition) - first;
        const glm::dvec3 rayDirection(direction);
        const glm::dvec3 offset = glm::dvec3(origin) - first;
        const glm::dvec3 perpendicular = glm::cross(rayDirection, edgeTwo);
        const double determinant = glm::dot(edgeOne, perpendicular);
        if (determinant == 0.0) {
            continue;
        }

        const double inverseDeterminant = 1.0 / determinant;
        const double firstBarycentric =
            glm::dot(offset, perpendicular) * inverseDeterminant;
        if (firstBarycentric < -barycentricTolerance ||
            firstBarycentric > 1.0 + barycentricTolerance) {
            continue;
        }

        const glm::dvec3 secondPerpendicular = glm::cross(offset, edgeOne);
        const double secondBarycentric =
            glm::dot(rayDirection, secondPerpendicular) * inverseDeterminant;
        if (secondBarycentric < -barycentricTolerance ||
            firstBarycentric + secondBarycentric > 1.0 + barycentricTolerance) {
            continue;
        }

        const double distance =
            glm::dot(edgeTwo, secondPerpendicular) * inverseDeterminant;
        if (std::isfinite(distance) &&
            distance >= static_cast<double>(minimumDistance) &&
            distance <= nearest) {
            nearest = distance;
            found = true;
        }
    }

    return found ? std::optional<float>(static_cast<float>(nearest)) : std::nullopt;
}

std::optional<float> intersectRayTransformedIndexedMesh(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const geometry::AxisAlignedBounds& localBounds,
    std::span<const glm::vec3> positions,
    std::span<const std::uint32_t> indices,
    const glm::mat4& localToWorld,
    float minimumDistance,
    float maximumDistance) {
    if (!isValid(localBounds)) {
        return std::nullopt;
    }

    const std::optional<LocalRay> ray = transformRay(origin, direction, localToWorld);
    if (!ray.has_value()) {
        return std::nullopt;
    }

    const std::optional<float> boundsHit =
        intersectRayAabb(ray->origin, ray->direction, localBounds);
    if (!boundsHit.has_value() || *boundsHit > maximumDistance) {
        return std::nullopt;
    }

    return intersectRayIndexedTriangles(
        ray->origin, ray->direction, positions, indices,
        minimumDistance, maximumDistance);
}

} // namespace core
