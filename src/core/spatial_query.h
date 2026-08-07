#ifndef CORE_SPATIAL_QUERY_H
#define CORE_SPATIAL_QUERY_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <glm/glm.hpp>
#include "math/geometry.h"

namespace core {

std::optional<geometry::AxisAlignedBounds> calculateIndexedBounds(
    std::span<const float> interleavedVertices,
    std::size_t componentsPerVertex,
    std::span<const std::uint32_t> indices);

std::optional<geometry::AxisAlignedBounds> mergeBounds(
    std::span<const geometry::AxisAlignedBounds> bounds);

std::optional<geometry::AxisAlignedBounds> transformBounds(
    const geometry::AxisAlignedBounds& localBounds,
    const glm::mat4& localToWorld);

std::optional<float> intersectRayAabb(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const geometry::AxisAlignedBounds& bounds);

std::optional<float> intersectRayTransformedAabb(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const geometry::AxisAlignedBounds& localBounds,
    const glm::mat4& localToWorld);

std::optional<float> intersectRayIndexedTriangles(
    const glm::vec3& origin,
    const glm::vec3& direction,
    std::span<const glm::vec3> positions,
    std::span<const std::uint32_t> indices,
    float minimumDistance,
    float maximumDistance);

std::optional<float> intersectRayTransformedIndexedMesh(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const geometry::AxisAlignedBounds& localBounds,
    std::span<const glm::vec3> positions,
    std::span<const std::uint32_t> indices,
    const glm::mat4& localToWorld,
    float minimumDistance,
    float maximumDistance);

} // namespace core

#endif // CORE_SPATIAL_QUERY_H
