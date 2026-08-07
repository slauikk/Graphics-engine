#include "editor_placement.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

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

std::optional<glm::vec3> findNearestFreeCubeGridPosition(
    const glm::vec3& basePosition,
    std::span<const geometry::AxisAlignedBounds> occupiedBounds,
    float maximumCenterCoordinate,
    int maximumRing,
    float padding) {
    if (!isFinite(basePosition) || !std::isfinite(maximumCenterCoordinate) ||
        maximumCenterCoordinate < 0.0f || maximumRing < 0 || maximumRing > 512 ||
        !std::isfinite(padding) || padding < 0.0f) {
        return std::nullopt;
    }

    const double coordinateLimit = static_cast<double>(maximumCenterCoordinate);
    const double maximumIntegralCoordinate =
        static_cast<double>((std::numeric_limits<std::int64_t>::max)()) -
        static_cast<double>(maximumRing) - 1.0;
    if (coordinateLimit > maximumIntegralCoordinate) {
        return std::nullopt;
    }
    const double reachableLimit = coordinateLimit + static_cast<double>(maximumRing);
    if (std::abs(static_cast<double>(basePosition.x)) > reachableLimit + 0.5 ||
        std::abs(static_cast<double>(basePosition.z)) > reachableLimit + 0.5) {
        return std::nullopt;
    }

    const std::int64_t baseX =
        static_cast<std::int64_t>(std::llround(static_cast<double>(basePosition.x)));
    const std::int64_t baseZ =
        static_cast<std::int64_t>(std::llround(static_cast<double>(basePosition.z)));
    const std::int64_t searchMinimumX = baseX - maximumRing;
    const std::int64_t searchMinimumZ = baseZ - maximumRing;
    const std::int64_t searchMaximumX = baseX + maximumRing;
    const std::int64_t searchMaximumZ = baseZ + maximumRing;

    const int side = maximumRing * 2 + 1;
    const int stride = side + 1;
    std::vector<int> occupancyDifference(
        static_cast<std::size_t>(stride) * static_cast<std::size_t>(stride), 0);
    const auto differenceAt = [&occupancyDifference, stride](int x, int z) -> int& {
        return occupancyDifference[
            static_cast<std::size_t>(z) * static_cast<std::size_t>(stride) +
            static_cast<std::size_t>(x)];
    };

    constexpr double cubeHalfExtent = 0.5;
    const double paddedExtent = cubeHalfExtent + static_cast<double>(padding);
    for (const geometry::AxisAlignedBounds& bounds : occupiedBounds) {
        if (!isValid(bounds) ||
            -paddedExtent > static_cast<double>(bounds.maximum.y) ||
            paddedExtent < static_cast<double>(bounds.minimum.y)) {
            continue;
        }

        const double minimumCellX =
            std::ceil(static_cast<double>(bounds.minimum.x) - paddedExtent);
        const double maximumCellX =
            std::floor(static_cast<double>(bounds.maximum.x) + paddedExtent);
        const double minimumCellZ =
            std::ceil(static_cast<double>(bounds.minimum.z) - paddedExtent);
        const double maximumCellZ =
            std::floor(static_cast<double>(bounds.maximum.z) + paddedExtent);
        if (maximumCellX < static_cast<double>(searchMinimumX) ||
            minimumCellX > static_cast<double>(searchMaximumX) ||
            maximumCellZ < static_cast<double>(searchMinimumZ) ||
            minimumCellZ > static_cast<double>(searchMaximumZ)) {
            continue;
        }

        const int minimumX = static_cast<int>(
            std::clamp(minimumCellX, static_cast<double>(searchMinimumX),
                       static_cast<double>(searchMaximumX)) -
            static_cast<double>(searchMinimumX));
        const int maximumX = static_cast<int>(
            std::clamp(maximumCellX, static_cast<double>(searchMinimumX),
                       static_cast<double>(searchMaximumX)) -
            static_cast<double>(searchMinimumX));
        const int minimumZ = static_cast<int>(
            std::clamp(minimumCellZ, static_cast<double>(searchMinimumZ),
                       static_cast<double>(searchMaximumZ)) -
            static_cast<double>(searchMinimumZ));
        const int maximumZ = static_cast<int>(
            std::clamp(maximumCellZ, static_cast<double>(searchMinimumZ),
                       static_cast<double>(searchMaximumZ)) -
            static_cast<double>(searchMinimumZ));

        ++differenceAt(minimumX, minimumZ);
        --differenceAt(maximumX + 1, minimumZ);
        --differenceAt(minimumX, maximumZ + 1);
        ++differenceAt(maximumX + 1, maximumZ + 1);
    }

    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            int value = differenceAt(x, z);
            if (x > 0) value += differenceAt(x - 1, z);
            if (z > 0) value += differenceAt(x, z - 1);
            if (x > 0 && z > 0) value -= differenceAt(x - 1, z - 1);
            differenceAt(x, z) = value;
        }
    }

    int bestDistanceSquared = (std::numeric_limits<int>::max)();
    std::optional<glm::vec3> bestPosition;
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            if (differenceAt(x, z) != 0) {
                continue;
            }
            const std::int64_t candidateX = searchMinimumX + x;
            const std::int64_t candidateZ = searchMinimumZ + z;
            if (std::abs(static_cast<double>(candidateX)) > coordinateLimit ||
                std::abs(static_cast<double>(candidateZ)) > coordinateLimit) {
                continue;
            }

            const int offsetX = x - maximumRing;
            const int offsetZ = z - maximumRing;
            const int distanceSquared = offsetX * offsetX + offsetZ * offsetZ;
            if (distanceSquared < bestDistanceSquared) {
                bestDistanceSquared = distanceSquared;
                bestPosition = glm::vec3(
                    static_cast<float>(candidateX), 0.0f,
                    static_cast<float>(candidateZ));
            }
        }
    }
    return bestPosition;
}

} // namespace core
