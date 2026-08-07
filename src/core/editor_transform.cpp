#include "editor_transform.h"

#include "scene_document.h"

#include <algorithm>
#include <cmath>

namespace core {
namespace {

bool isFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

bool componentsInRange(const glm::vec3& value, float minimum, float maximum) {
    return isFinite(value) && value.x >= minimum && value.x <= maximum &&
           value.y >= minimum && value.y <= maximum &&
           value.z >= minimum && value.z <= maximum;
}

bool isValid(const ObjectTransform& transform) {
    return componentsInRange(
               transform.position, -kMaxSceneCoordinate, kMaxSceneCoordinate) &&
           componentsInRange(
               transform.rotationDeg, -kMaxSceneCoordinate, kMaxSceneCoordinate) &&
           componentsInRange(
               transform.scale, kMinSceneObjectScale, kMaxSceneObjectScale);
}

bool equal(const glm::vec3& left, const glm::vec3& right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

float wrapDegrees(float value) {
    const float wrapped = std::remainder(value, 360.0f);
    return wrapped == 0.0f ? 0.0f : wrapped;
}

float snapCoordinate(float value, float step) {
    const double doubleStep = static_cast<double>(step);
    const double maximumGridIndex = std::floor(
        static_cast<double>(kMaxSceneCoordinate) / doubleStep);
    const double gridIndex = std::clamp(
        std::round(static_cast<double>(value) / doubleStep),
        -maximumGridIndex, maximumGridIndex);
    return static_cast<float>(gridIndex * doubleStep);
}

} // namespace

glm::vec3 wrapEulerDegrees(const glm::vec3& rotationDegrees) {
    return glm::vec3(
        wrapDegrees(rotationDegrees.x),
        wrapDegrees(rotationDegrees.y),
        wrapDegrees(rotationDegrees.z));
}

bool isRepeatableObjectTransform(ObjectTransformCommand command) {
    switch (command) {
        case ObjectTransformCommand::MoveNegativeX:
        case ObjectTransformCommand::MovePositiveX:
        case ObjectTransformCommand::MoveNegativeY:
        case ObjectTransformCommand::MovePositiveY:
        case ObjectTransformCommand::MoveNegativeZ:
        case ObjectTransformCommand::MovePositiveZ:
        case ObjectTransformCommand::RotateNegativeY:
        case ObjectTransformCommand::RotatePositiveY:
        case ObjectTransformCommand::ScaleDown:
        case ObjectTransformCommand::ScaleUp:
            return true;
        case ObjectTransformCommand::Snap:
        case ObjectTransformCommand::Reset:
            return false;
    }
    return false;
}

std::optional<ObjectTransform> calculateObjectTransform(
    const ObjectTransform& current,
    ObjectTransformCommand command,
    float translationStep,
    float rotationStepDegrees,
    float scaleFactor) {
    if (!isValid(current) || !std::isfinite(translationStep) ||
        translationStep <= 0.0f || !std::isfinite(rotationStepDegrees) ||
        rotationStepDegrees <= 0.0f || !std::isfinite(scaleFactor) ||
        scaleFactor <= 1.0f) {
        return std::nullopt;
    }

    ObjectTransform result = current;
    switch (command) {
        case ObjectTransformCommand::MoveNegativeX:
            result.position.x -= translationStep;
            break;
        case ObjectTransformCommand::MovePositiveX:
            result.position.x += translationStep;
            break;
        case ObjectTransformCommand::MoveNegativeY:
            result.position.y -= translationStep;
            break;
        case ObjectTransformCommand::MovePositiveY:
            result.position.y += translationStep;
            break;
        case ObjectTransformCommand::MoveNegativeZ:
            result.position.z -= translationStep;
            break;
        case ObjectTransformCommand::MovePositiveZ:
            result.position.z += translationStep;
            break;
        case ObjectTransformCommand::RotateNegativeY:
            result.rotationDeg.y = wrapDegrees(
                result.rotationDeg.y - rotationStepDegrees);
            break;
        case ObjectTransformCommand::RotatePositiveY:
            result.rotationDeg.y = wrapDegrees(
                result.rotationDeg.y + rotationStepDegrees);
            break;
        case ObjectTransformCommand::ScaleDown: {
            const double minimumFactor = std::max({
                static_cast<double>(kMinSceneObjectScale) / result.scale.x,
                static_cast<double>(kMinSceneObjectScale) / result.scale.y,
                static_cast<double>(kMinSceneObjectScale) / result.scale.z});
            const double boundedFactor = std::max(
                1.0 / static_cast<double>(scaleFactor), minimumFactor);
            result.scale = glm::vec3(
                static_cast<float>(result.scale.x * boundedFactor),
                static_cast<float>(result.scale.y * boundedFactor),
                static_cast<float>(result.scale.z * boundedFactor));
            break;
        }
        case ObjectTransformCommand::ScaleUp: {
            const double maximumFactor = std::min({
                static_cast<double>(kMaxSceneObjectScale) / result.scale.x,
                static_cast<double>(kMaxSceneObjectScale) / result.scale.y,
                static_cast<double>(kMaxSceneObjectScale) / result.scale.z});
            const double boundedFactor = std::min(
                static_cast<double>(scaleFactor), maximumFactor);
            result.scale = glm::vec3(
                static_cast<float>(result.scale.x * boundedFactor),
                static_cast<float>(result.scale.y * boundedFactor),
                static_cast<float>(result.scale.z * boundedFactor));
            break;
        }
        case ObjectTransformCommand::Snap:
            result.position = glm::vec3(
                snapCoordinate(result.position.x, translationStep),
                snapCoordinate(result.position.y, translationStep),
                snapCoordinate(result.position.z, translationStep));
            result.rotationDeg = wrapEulerDegrees(
                glm::round(result.rotationDeg / rotationStepDegrees) *
                rotationStepDegrees);
            break;
        case ObjectTransformCommand::Reset:
            result.position = glm::vec3(0.0f);
            result.rotationDeg = glm::vec3(0.0f);
            result.scale = glm::vec3(1.0f);
            break;
    }

    if (!isValid(result) ||
        (equal(result.position, current.position) &&
         equal(result.rotationDeg, current.rotationDeg) &&
         equal(result.scale, current.scale))) {
        return std::nullopt;
    }
    return result;
}

} // namespace core
