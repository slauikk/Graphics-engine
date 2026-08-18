#include "editor_gizmo.h"

#include "scene_document.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace {

constexpr float kMinimumProjectedAxisLength = 2.0f;
constexpr float kMinimumProjectedRingExtent = 8.0f;
constexpr float kMinimumRayPlaneDenominator = 0.00001f;
constexpr float kMinimumRadialLength = 0.00001f;

std::optional<core::EditorPoint> projectToViewport(
    const core::EditorRect& viewport,
    const glm::vec3& worldPoint,
    const glm::mat4& viewProjection) {
    const glm::vec4 clip =
        viewProjection * glm::vec4(worldPoint, 1.0f);
    if (!std::isfinite(clip.x) || !std::isfinite(clip.y) ||
        !std::isfinite(clip.w) || clip.w <= 0.0001f) {
        return std::nullopt;
    }

    const float normalizedX = clip.x / clip.w;
    const float normalizedY = clip.y / clip.w;
    const float normalizedZ = clip.z / clip.w;
    if (!std::isfinite(normalizedX) || !std::isfinite(normalizedY) ||
        !std::isfinite(normalizedZ) || normalizedZ < -1.0f ||
        normalizedZ > 1.0f) {
        return std::nullopt;
    }
    return core::EditorPoint{
        static_cast<double>(viewport.x) +
            (static_cast<double>(normalizedX) * 0.5 + 0.5) *
                static_cast<double>(viewport.width),
        static_cast<double>(viewport.y) +
            (0.5 - static_cast<double>(normalizedY) * 0.5) *
                static_cast<double>(viewport.height)};
}

std::optional<std::size_t> axisIndex(core::EditorGizmoAxis axis) {
    switch (axis) {
        case core::EditorGizmoAxis::X:
            return 0;
        case core::EditorGizmoAxis::Y:
            return 1;
        case core::EditorGizmoAxis::Z:
            return 2;
        case core::EditorGizmoAxis::None:
            return std::nullopt;
    }
    return std::nullopt;
}

glm::vec3 axisVector(std::size_t index) {
    switch (index) {
        case 0:
            return {1.0f, 0.0f, 0.0f};
        case 1:
            return {0.0f, 1.0f, 0.0f};
        default:
            return {0.0f, 0.0f, 1.0f};
    }
}

bool finitePoint(core::EditorPoint point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool finiteVector(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

bool finiteMatrix(const glm::mat4& value) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (!std::isfinite(value[column][row])) {
                return false;
            }
        }
    }
    return true;
}

std::pair<glm::vec3, glm::vec3> rotationPlaneBasis(std::size_t index) {
    switch (index) {
        case 0:
            return {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
        case 1:
            return {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
        default:
            return {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    }
}

double distanceSquaredToSegment(
    core::EditorPoint point,
    core::EditorPoint start,
    core::EditorPoint end) {
    const double segmentX = end.x - start.x;
    const double segmentY = end.y - start.y;
    const double lengthSquared =
        segmentX * segmentX + segmentY * segmentY;
    if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0) {
        const double deltaX = point.x - start.x;
        const double deltaY = point.y - start.y;
        return deltaX * deltaX + deltaY * deltaY;
    }
    const double projection = std::clamp(
        ((point.x - start.x) * segmentX +
         (point.y - start.y) * segmentY) / lengthSquared,
        0.0,
        1.0);
    const double nearestX = start.x + projection * segmentX;
    const double nearestY = start.y + projection * segmentY;
    const double deltaX = point.x - nearestX;
    const double deltaY = point.y - nearestY;
    return deltaX * deltaX + deltaY * deltaY;
}

std::optional<glm::vec3> rotationPlaneDirectionAt(
    const core::EditorRotationGizmo& gizmo,
    std::size_t axis,
    core::EditorPoint cursor) {
    if (!gizmo.viewport.valid() || !finitePoint(cursor) ||
        axis >= gizmo.axesValid.size() || !gizmo.axesValid[axis] ||
        !finiteMatrix(gizmo.inverseViewProjection)) {
        return std::nullopt;
    }

    const float normalizedX = static_cast<float>(
        2.0 * (cursor.x - static_cast<double>(gizmo.viewport.x)) /
            static_cast<double>(gizmo.viewport.width) -
        1.0);
    const float normalizedY = static_cast<float>(
        1.0 - 2.0 *
            (cursor.y - static_cast<double>(gizmo.viewport.y)) /
            static_cast<double>(gizmo.viewport.height));
    const glm::vec4 nearHomogeneous = gizmo.inverseViewProjection *
        glm::vec4(normalizedX, normalizedY, -1.0f, 1.0f);
    const glm::vec4 farHomogeneous = gizmo.inverseViewProjection *
        glm::vec4(normalizedX, normalizedY, 1.0f, 1.0f);
    if (!std::isfinite(nearHomogeneous.w) ||
        !std::isfinite(farHomogeneous.w) ||
        std::abs(nearHomogeneous.w) <= kMinimumRadialLength ||
        std::abs(farHomogeneous.w) <= kMinimumRadialLength) {
        return std::nullopt;
    }
    const glm::vec3 nearPoint = glm::vec3(nearHomogeneous) /
        nearHomogeneous.w;
    const glm::vec3 farPoint = glm::vec3(farHomogeneous) /
        farHomogeneous.w;
    const glm::vec3 rayDelta = farPoint - nearPoint;
    const float rayLength = glm::length(rayDelta);
    if (!finiteVector(nearPoint) || !finiteVector(farPoint) ||
        !std::isfinite(rayLength) || rayLength <= kMinimumRadialLength) {
        return std::nullopt;
    }
    const glm::vec3 rayDirection = rayDelta / rayLength;
    const glm::vec3 normal = axisVector(axis);
    const float denominator = glm::dot(rayDirection, normal);
    if (!std::isfinite(denominator) ||
        std::abs(denominator) <= kMinimumRayPlaneDenominator) {
        return std::nullopt;
    }
    const float distance = glm::dot(
        gizmo.worldOrigin - nearPoint, normal) / denominator;
    if (!std::isfinite(distance) || distance < 0.0f) {
        return std::nullopt;
    }
    const glm::vec3 radial =
        nearPoint + rayDirection * distance - gizmo.worldOrigin;
    const float radialLength = glm::length(radial);
    if (!finiteVector(radial) || !std::isfinite(radialLength) ||
        radialLength <= kMinimumRadialLength) {
        return std::nullopt;
    }
    return radial / radialLength;
}

float snapCoordinate(float value, float step) {
    const double doubleStep = static_cast<double>(step);
    const double maximumGridIndex = std::floor(
        static_cast<double>(core::kMaxSceneCoordinate) / doubleStep);
    const double gridIndex = std::clamp(
        std::round(static_cast<double>(value) / doubleStep),
        -maximumGridIndex, maximumGridIndex);
    return static_cast<float>(gridIndex * doubleStep);
}

} // namespace

namespace core {

EditorTranslationGizmo calculateEditorTranslationGizmo(
    const EditorRect& viewport,
    const glm::vec3& worldOrigin,
    const glm::mat4& view,
    const glm::mat4& projection,
    float axisLengthPixels,
    int handleSize) {
    EditorTranslationGizmo gizmo;
    if (!viewport.valid() || !finiteVector(worldOrigin) ||
        !std::isfinite(axisLengthPixels) || axisLengthPixels <= 0.0f ||
        handleSize <= 0) {
        return gizmo;
    }

    const glm::mat4 viewProjection = projection * view;
    const std::optional<EditorPoint> origin =
        projectToViewport(viewport, worldOrigin, viewProjection);
    if (!origin.has_value() || !viewport.contains(*origin)) {
        return gizmo;
    }
    gizmo.origin = *origin;

    for (std::size_t index = 0; index < gizmo.endpoints.size(); ++index) {
        const std::optional<EditorPoint> projectedAxis = projectToViewport(
            viewport, worldOrigin + axisVector(index), viewProjection);
        if (!projectedAxis.has_value()) {
            continue;
        }

        const glm::vec2 projectedDelta(
            static_cast<float>(projectedAxis->x - origin->x),
            static_cast<float>(projectedAxis->y - origin->y));
        const float projectedLength = glm::length(projectedDelta);
        if (!std::isfinite(projectedLength) ||
            projectedLength < kMinimumProjectedAxisLength) {
            continue;
        }

        const glm::vec2 direction = projectedDelta / projectedLength;
        const EditorPoint endpoint{
            origin->x + static_cast<double>(direction.x * axisLengthPixels),
            origin->y + static_cast<double>(direction.y * axisLengthPixels)};
        const int handleX = static_cast<int>(std::lround(endpoint.x)) -
            handleSize / 2;
        const int handleY = static_cast<int>(std::lround(endpoint.y)) -
            handleSize / 2;
        const EditorRect handle{
            handleX, handleY, handleSize, handleSize};
        if (!viewport.contains({
                static_cast<double>(handle.x),
                static_cast<double>(handle.y)}) ||
            !viewport.contains({
                static_cast<double>(handle.x + handle.width - 1),
                static_cast<double>(handle.y + handle.height - 1)})) {
            continue;
        }

        gizmo.endpoints[index] = endpoint;
        gizmo.handles[index] = handle;
        gizmo.screenDirections[index] = direction;
        gizmo.worldUnitsPerPixel[index] = 1.0f / projectedLength;
        gizmo.valid = true;
    }
    return gizmo;
}

EditorGizmoAxis editorGizmoAxisAt(
    const EditorTranslationGizmo& gizmo,
    EditorPoint point) {
    if (!gizmo.valid || !finitePoint(point)) {
        return EditorGizmoAxis::None;
    }
    constexpr std::array<EditorGizmoAxis, 3> axes = {
        EditorGizmoAxis::X,
        EditorGizmoAxis::Y,
        EditorGizmoAxis::Z};
    EditorGizmoAxis nearestAxis = EditorGizmoAxis::None;
    double nearestDistanceSquared =
        (std::numeric_limits<double>::max)();
    for (std::size_t index = 0; index < axes.size(); ++index) {
        const EditorRect& handle = gizmo.handles[index];
        const EditorRect hitArea{
            handle.x - kEditorGizmoHitPadding,
            handle.y - kEditorGizmoHitPadding,
            handle.width + 2 * kEditorGizmoHitPadding,
            handle.height + 2 * kEditorGizmoHitPadding};
        if (handle.valid() && hitArea.contains(point)) {
            const double centerX = handle.x + handle.width * 0.5;
            const double centerY = handle.y + handle.height * 0.5;
            const double deltaX = point.x - centerX;
            const double deltaY = point.y - centerY;
            const double distanceSquared =
                deltaX * deltaX + deltaY * deltaY;
            if (distanceSquared < nearestDistanceSquared) {
                nearestAxis = axes[index];
                nearestDistanceSquared = distanceSquared;
            }
        }
    }
    return nearestAxis;
}

std::optional<glm::vec3> calculateEditorGizmoTranslation(
    const EditorTranslationGizmo& gizmo,
    EditorGizmoAxis axis,
    const glm::vec3& startPosition,
    EditorPoint startCursor,
    EditorPoint currentCursor,
    float snapStep) {
    const std::optional<std::size_t> index = axisIndex(axis);
    if (!gizmo.valid || !index.has_value() ||
        !gizmo.handles[*index].valid() || !finiteVector(startPosition) ||
        !finitePoint(startCursor) || !finitePoint(currentCursor) ||
        !std::isfinite(snapStep) || snapStep < 0.0f) {
        return std::nullopt;
    }

    const glm::vec2 cursorDelta(
        static_cast<float>(currentCursor.x - startCursor.x),
        static_cast<float>(currentCursor.y - startCursor.y));
    const float pixelDistance = glm::dot(
        cursorDelta, gizmo.screenDirections[*index]);
    const float worldDistance =
        pixelDistance * gizmo.worldUnitsPerPixel[*index];
    if (!std::isfinite(worldDistance)) {
        return std::nullopt;
    }

    glm::vec3 translated =
        startPosition + axisVector(*index) * worldDistance;
    if (snapStep > 0.0f) {
        switch (*index) {
            case 0:
                translated.x = snapCoordinate(translated.x, snapStep);
                break;
            case 1:
                translated.y = snapCoordinate(translated.y, snapStep);
                break;
            default:
                translated.z = snapCoordinate(translated.z, snapStep);
                break;
        }
    }
    translated.x = std::clamp(
        translated.x, -kMaxSceneCoordinate, kMaxSceneCoordinate);
    translated.y = std::clamp(
        translated.y, -kMaxSceneCoordinate, kMaxSceneCoordinate);
    translated.z = std::clamp(
        translated.z, -kMaxSceneCoordinate, kMaxSceneCoordinate);
    return translated;
}

bool editorGizmoTranslationChanged(
    const glm::vec3& startPosition,
    const glm::vec3& currentPosition) {
    return finiteVector(startPosition) && finiteVector(currentPosition) &&
        (startPosition.x != currentPosition.x ||
         startPosition.y != currentPosition.y ||
         startPosition.z != currentPosition.z);
}

EditorRotationGizmo calculateEditorRotationGizmo(
    const EditorRect& viewport,
    const glm::vec3& worldOrigin,
    const glm::mat4& view,
    const glm::mat4& projection,
    float radiusPixels) {
    EditorRotationGizmo gizmo;
    if (!viewport.valid() || !finiteVector(worldOrigin) ||
        !finiteMatrix(view) || !finiteMatrix(projection) ||
        !std::isfinite(radiusPixels) || radiusPixels <= 0.0f) {
        return gizmo;
    }

    const glm::mat4 viewProjection = projection * view;
    const float determinant = glm::determinant(viewProjection);
    if (!std::isfinite(determinant) ||
        std::abs(determinant) <= std::numeric_limits<float>::epsilon()) {
        return gizmo;
    }
    const std::optional<EditorPoint> origin =
        projectToViewport(viewport, worldOrigin, viewProjection);
    if (!origin.has_value() || !viewport.contains(*origin)) {
        return gizmo;
    }

    gizmo.viewport = viewport;
    gizmo.origin = *origin;
    gizmo.worldOrigin = worldOrigin;
    gizmo.inverseViewProjection = glm::inverse(viewProjection);
    if (!finiteMatrix(gizmo.inverseViewProjection)) {
        return {};
    }

    for (std::size_t axis = 0; axis < gizmo.rings.size(); ++axis) {
        const auto [basisU, basisV] = rotationPlaneBasis(axis);
        const auto projectedU = projectToViewport(
            viewport, worldOrigin + basisU, viewProjection);
        const auto projectedV = projectToViewport(
            viewport, worldOrigin + basisV, viewProjection);
        if (!projectedU.has_value() || !projectedV.has_value()) {
            continue;
        }
        const float projectedULength = glm::length(glm::vec2(
            static_cast<float>(projectedU->x - origin->x),
            static_cast<float>(projectedU->y - origin->y)));
        const float projectedVLength = glm::length(glm::vec2(
            static_cast<float>(projectedV->x - origin->x),
            static_cast<float>(projectedV->y - origin->y)));
        const float maximumProjectedLength = (std::max)(
            projectedULength, projectedVLength);
        if (!std::isfinite(maximumProjectedLength) ||
            maximumProjectedLength < kMinimumProjectedAxisLength) {
            continue;
        }
        const float worldRadius = radiusPixels / maximumProjectedLength;
        double minimumX = (std::numeric_limits<double>::max)();
        double minimumY = (std::numeric_limits<double>::max)();
        double maximumX = (std::numeric_limits<double>::lowest)();
        double maximumY = (std::numeric_limits<double>::lowest)();
        bool completeRing = true;
        for (std::size_t pointIndex = 0;
             pointIndex <= kEditorRotationGizmoSegmentCount;
             ++pointIndex) {
            const float angle = glm::two_pi<float>() *
                static_cast<float>(pointIndex) /
                static_cast<float>(kEditorRotationGizmoSegmentCount);
            const glm::vec3 worldPoint = worldOrigin + worldRadius *
                (basisU * std::cos(angle) + basisV * std::sin(angle));
            const auto projected = projectToViewport(
                viewport, worldPoint, viewProjection);
            if (!projected.has_value() || !viewport.contains(*projected)) {
                completeRing = false;
                break;
            }
            gizmo.rings[axis][pointIndex] = *projected;
            minimumX = (std::min)(minimumX, projected->x);
            minimumY = (std::min)(minimumY, projected->y);
            maximumX = (std::max)(maximumX, projected->x);
            maximumY = (std::max)(maximumY, projected->y);
        }
        if (!completeRing ||
            maximumX - minimumX < kMinimumProjectedRingExtent ||
            maximumY - minimumY < kMinimumProjectedRingExtent) {
            gizmo.rings[axis] = {};
            continue;
        }
        gizmo.axesValid[axis] = true;
        gizmo.valid = true;
    }
    return gizmo;
}

EditorGizmoAxis editorRotationGizmoAxisAt(
    const EditorRotationGizmo& gizmo,
    EditorPoint point) {
    if (!gizmo.valid || !finitePoint(point)) {
        return EditorGizmoAxis::None;
    }
    constexpr std::array<EditorGizmoAxis, 3> axes = {
        EditorGizmoAxis::X,
        EditorGizmoAxis::Y,
        EditorGizmoAxis::Z};
    const double hitDistanceSquared =
        static_cast<double>(kEditorRotationGizmoHitDistancePixels) *
        static_cast<double>(kEditorRotationGizmoHitDistancePixels);
    double nearestDistanceSquared = hitDistanceSquared;
    EditorGizmoAxis nearestAxis = EditorGizmoAxis::None;
    for (std::size_t axis = 0; axis < axes.size(); ++axis) {
        if (!gizmo.axesValid[axis]) {
            continue;
        }
        for (std::size_t segment = 0;
             segment < kEditorRotationGizmoSegmentCount;
             ++segment) {
            const double distanceSquared = distanceSquaredToSegment(
                point,
                gizmo.rings[axis][segment],
                gizmo.rings[axis][segment + 1]);
            if (distanceSquared <= nearestDistanceSquared) {
                nearestDistanceSquared = distanceSquared;
                nearestAxis = axes[axis];
            }
        }
    }
    return nearestAxis;
}

std::optional<glm::vec3> calculateEditorGizmoRotation(
    const EditorRotationGizmo& gizmo,
    EditorGizmoAxis axis,
    const glm::vec3& startRotationDegrees,
    EditorPoint startCursor,
    EditorPoint currentCursor,
    float snapStepDegrees) {
    const auto index = axisIndex(axis);
    if (!gizmo.valid || !index.has_value() ||
        !gizmo.axesValid[*index] || !finiteVector(startRotationDegrees) ||
        !finitePoint(startCursor) || !finitePoint(currentCursor) ||
        !std::isfinite(snapStepDegrees) || snapStepDegrees < 0.0f) {
        return std::nullopt;
    }
    const auto startDirection = rotationPlaneDirectionAt(
        gizmo, *index, startCursor);
    const auto currentDirection = rotationPlaneDirectionAt(
        gizmo, *index, currentCursor);
    if (!startDirection.has_value() || !currentDirection.has_value()) {
        return std::nullopt;
    }
    const glm::vec3 normal = axisVector(*index);
    const float sine = glm::dot(
        normal, glm::cross(*startDirection, *currentDirection));
    const float cosine = std::clamp(
        glm::dot(*startDirection, *currentDirection), -1.0f, 1.0f);
    const float deltaDegrees = glm::degrees(std::atan2(sine, cosine));
    if (!std::isfinite(deltaDegrees)) {
        return std::nullopt;
    }

    glm::vec3 rotated = startRotationDegrees;
    float& rotationComponent = *index == 0
        ? rotated.x
        : (*index == 1 ? rotated.y : rotated.z);
    rotationComponent += deltaDegrees;
    if (snapStepDegrees > 0.0f) {
        rotationComponent = static_cast<float>(
            std::round(
                static_cast<double>(rotationComponent) /
                static_cast<double>(snapStepDegrees)) *
            static_cast<double>(snapStepDegrees));
    }
    return wrapEulerDegrees(rotated);
}

bool editorGizmoRotationChanged(
    const glm::vec3& startRotationDegrees,
    const glm::vec3& currentRotationDegrees) {
    if (!finiteVector(startRotationDegrees) ||
        !finiteVector(currentRotationDegrees)) {
        return false;
    }
    constexpr float epsilon = 0.0001f;
    return std::abs(startRotationDegrees.x - currentRotationDegrees.x) >
            epsilon ||
        std::abs(startRotationDegrees.y - currentRotationDegrees.y) >
            epsilon ||
        std::abs(startRotationDegrees.z - currentRotationDegrees.z) >
            epsilon;
}

const char* editorGizmoAxisName(EditorGizmoAxis axis) {
    switch (axis) {
        case EditorGizmoAxis::X:
            return "X";
        case EditorGizmoAxis::Y:
            return "Y";
        case EditorGizmoAxis::Z:
            return "Z";
        case EditorGizmoAxis::None:
            return "";
    }
    return "";
}

} // namespace core
