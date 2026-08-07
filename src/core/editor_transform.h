#ifndef CORE_EDITOR_TRANSFORM_H
#define CORE_EDITOR_TRANSFORM_H

#include <optional>

#include <glm/glm.hpp>

namespace core {

inline constexpr float kObjectTranslationStep = 0.5f;
inline constexpr float kObjectRotationStepDegrees = 15.0f;
inline constexpr float kObjectScaleFactor = 1.1f;

enum class ObjectTransformCommand {
    MoveNegativeX,
    MovePositiveX,
    MoveNegativeY,
    MovePositiveY,
    MoveNegativeZ,
    MovePositiveZ,
    RotateNegativeY,
    RotatePositiveY,
    ScaleDown,
    ScaleUp,
    Snap,
    Reset
};

struct ObjectTransform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotationDeg = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

glm::vec3 wrapEulerDegrees(const glm::vec3& rotationDegrees);

bool isRepeatableObjectTransform(ObjectTransformCommand command);

std::optional<ObjectTransform> calculateObjectTransform(
    const ObjectTransform& current,
    ObjectTransformCommand command,
    float translationStep = kObjectTranslationStep,
    float rotationStepDegrees = kObjectRotationStepDegrees,
    float scaleFactor = kObjectScaleFactor);

} // namespace core

#endif // CORE_EDITOR_TRANSFORM_H
