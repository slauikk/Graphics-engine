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

const char* editorGizmoAxisName(EditorGizmoAxis axis);

} // namespace core

#endif // CORE_EDITOR_GIZMO_H
