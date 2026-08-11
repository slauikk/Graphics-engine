#include "editor_gizmo.h"

#include "scene_document.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace {

constexpr float kMinimumProjectedAxisLength = 2.0f;

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
