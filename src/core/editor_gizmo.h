#ifndef CORE_EDITOR_GIZMO_H
#define CORE_EDITOR_GIZMO_H

#include "editor_layout.h"

#include <array>
#include <optional>

#include <glm/glm.hpp>

namespace core {

inline constexpr float kEditorGizmoAxisLengthPixels = 56.0f;
inline constexpr int kEditorGizmoHandleSize = 12;
inline constexpr int kEditorGizmoHitPadding = 8;
inline constexpr float kEditorRotationGizmoRadiusPixels = 54.0f;
inline constexpr float kEditorRotationGizmoHitDistancePixels = 7.0f;
inline constexpr std::size_t kEditorRotationGizmoSegmentCount = 64;

enum class EditorGizmoMode {
    Translate,
    Rotate
};

enum class EditorGizmoAxis {
    None,
    X,
    Y,
    Z
};

struct EditorTranslationGizmo {
    bool valid = false;
    EditorPoint origin;
    std::array<EditorPoint, 3> endpoints;
    std::array<EditorRect, 3> handles;
    std::array<glm::vec2, 3> screenDirections{};
    std::array<float, 3> worldUnitsPerPixel{};
};

struct EditorRotationGizmo {
    bool valid = false;
    EditorRect viewport;
    EditorPoint origin;
    glm::vec3 worldOrigin = glm::vec3(0.0f);
    glm::mat4 inverseViewProjection = glm::mat4(1.0f);
    std::array<
        std::array<EditorPoint, kEditorRotationGizmoSegmentCount + 1>,
        3> rings;
    std::array<bool, 3> axesValid{};
};

EditorTranslationGizmo calculateEditorTranslationGizmo(
    const EditorRect& viewport,
    const glm::vec3& worldOrigin,
    const glm::mat4& view,
    const glm::mat4& projection,
    float axisLengthPixels = kEditorGizmoAxisLengthPixels,
    int handleSize = kEditorGizmoHandleSize);

EditorGizmoAxis editorGizmoAxisAt(
    const EditorTranslationGizmo& gizmo,
    EditorPoint point);

std::optional<glm::vec3> calculateEditorGizmoTranslation(
    const EditorTranslationGizmo& gizmo,
    EditorGizmoAxis axis,
    const glm::vec3& startPosition,
    EditorPoint startCursor,
    EditorPoint currentCursor,
    float snapStep = 0.0f);

bool editorGizmoTranslationChanged(
    const glm::vec3& startPosition,
    const glm::vec3& currentPosition);

EditorRotationGizmo calculateEditorRotationGizmo(
    const EditorRect& viewport,
    const glm::vec3& worldOrigin,
    const glm::mat4& view,
    const glm::mat4& projection,
    float radiusPixels = kEditorRotationGizmoRadiusPixels);

EditorGizmoAxis editorRotationGizmoAxisAt(
    const EditorRotationGizmo& gizmo,
    EditorPoint point);

std::optional<glm::vec3> calculateEditorGizmoRotation(
    const EditorRotationGizmo& gizmo,
    EditorGizmoAxis axis,
    const glm::vec3& startRotationDegrees,
    EditorPoint startCursor,
    EditorPoint currentCursor,
    float snapStepDegrees = 0.0f);

bool editorGizmoRotationChanged(
    const glm::vec3& startRotationDegrees,
    const glm::vec3& currentRotationDegrees);

const char* editorGizmoAxisName(EditorGizmoAxis axis);

} // namespace core

#endif // CORE_EDITOR_GIZMO_H
