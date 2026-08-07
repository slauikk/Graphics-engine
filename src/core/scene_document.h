#ifndef SCENE_DOCUMENT_H
#define SCENE_DOCUMENT_H

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace core {

inline constexpr int kCurrentSceneSchemaVersion = 1;
inline constexpr std::size_t kMaxSceneObjectCount = 100'000;
inline constexpr std::size_t kMaxSceneObjectNameLength = 128;

struct SceneMaterial {
    std::string id;
    std::string albedoTexture;
    glm::vec3 albedoColor = glm::vec3(1.0f);
    glm::vec3 specularColor = glm::vec3(0.5f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float shininess = 32.0f;
    float metallic = 0.0f;
    float roughness = 0.0f;
};

struct SceneObject {
    std::string name;
    std::string mesh = "builtin:cube";
    std::string material;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotationDeg = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    bool spinning = false;
};

struct SceneCamera {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 45.0f;
    float movementSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
};

struct SceneDirectionalLight {
    glm::vec3 direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
    glm::vec3 color = glm::vec3(1.0f);
    bool enabled = true;
};

struct ScenePointLight {
    glm::vec3 position = glm::vec3(2.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    bool enabled = true;
    bool spinning = false;
};

struct SceneRenderSettings {
    int postProcessEffect = 0;
    int shaderViewMode = 0;
    bool coordinateGrid = true;
};

struct SceneDocument {
    int schemaVersion = kCurrentSceneSchemaVersion;
    SceneCamera camera;
    SceneDirectionalLight directionalLight;
    ScenePointLight pointLight;
    SceneRenderSettings renderSettings;
    std::vector<SceneMaterial> materials;
    std::vector<SceneObject> objects;
    int selectedObject = -1;
};

struct SceneIoResult {
    bool success = false;
    std::string error;
};

bool validateSceneDocument(const SceneDocument& scene, std::string& error);
SceneIoResult saveSceneDocument(
    const SceneDocument& scene,
    const std::filesystem::path& path);
SceneIoResult loadSceneDocument(
    const std::filesystem::path& path,
    SceneDocument& scene);

} // namespace core

#endif // SCENE_DOCUMENT_H
