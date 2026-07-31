#include "scene_document.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <unordered_set>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

using Json = nlohmann::json;

constexpr std::size_t kMaxMaterials = 32'768;
constexpr std::size_t kMaxObjects = 100'000;
constexpr std::size_t kMaxIdLength = 128;
constexpr std::size_t kMaxAssetReferenceLength = 1'024;
constexpr std::uintmax_t kMaxSceneFileSize = 32U * 1024U * 1024U;
constexpr float kMaxCoordinate = 1'000'000.0f;
constexpr float kMaxColorComponent = 100.0f;

Json vec3ToJson(const glm::vec3& value) {
    return Json::array({value.x, value.y, value.z});
}

glm::vec3 vec3FromJson(const Json& value) {
    if (!value.is_array() || value.size() != 3) {
        throw Json::type_error::create(302, "expected an array with three numbers", &value);
    }
    return glm::vec3(
        value.at(0).get<float>(),
        value.at(1).get<float>(),
        value.at(2).get<float>());
}

bool isFinite(float value) {
    return std::isfinite(value);
}

bool isFinite(const glm::vec3& value) {
    return isFinite(value.x) && isFinite(value.y) && isFinite(value.z);
}

bool componentsInRange(const glm::vec3& value, float minimum, float maximum) {
    return isFinite(value) && value.x >= minimum && value.x <= maximum &&
           value.y >= minimum && value.y <= maximum &&
           value.z >= minimum && value.z <= maximum;
}

bool coordinatesInRange(const glm::vec3& value) {
    return componentsInRange(value, -kMaxCoordinate, kMaxCoordinate);
}

bool isValidText(std::string_view value, std::size_t maximumLength, bool allowEmpty) {
    if ((!allowEmpty && value.empty()) || value.size() > maximumLength) {
        return false;
    }
    for (const unsigned char character : value) {
        if (character < 0x20U || character == 0x7fU) {
            return false;
        }
    }
    return true;
}

bool isSafeAssetReference(std::string_view value, bool allowEmpty) {
    if (value.empty()) {
        return allowEmpty;
    }
    if (!isValidText(value, kMaxAssetReferenceLength, false) ||
        value.find('\\') != std::string_view::npos) {
        return false;
    }
    if (value.starts_with("builtin:")) {
        return value.size() > std::string_view("builtin:").size() &&
               value.find('/', std::string_view("builtin:").size()) == std::string_view::npos;
    }
    if (value.find(':') != std::string_view::npos) {
        return false;
    }

    const std::filesystem::path path(value);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return false;
    }
    for (const auto& component : path) {
        if (component == ".." || component == ".") {
            return false;
        }
    }
    return true;
}

bool setValidationError(std::string& error, std::string message) {
    error = std::move(message);
    return false;
}

Json materialToJson(const core::SceneMaterial& material) {
    return {
        {"id", material.id},
        {"albedo_texture", material.albedoTexture},
        {"albedo_color", vec3ToJson(material.albedoColor)},
        {"specular_color", vec3ToJson(material.specularColor)},
        {"emissive_color", vec3ToJson(material.emissiveColor)},
        {"shininess", material.shininess},
        {"metallic", material.metallic},
        {"roughness", material.roughness}
    };
}

core::SceneMaterial materialFromJson(const Json& value) {
    core::SceneMaterial material;
    material.id = value.at("id").get<std::string>();
    material.albedoTexture = value.at("albedo_texture").get<std::string>();
    material.albedoColor = vec3FromJson(value.at("albedo_color"));
    material.specularColor = vec3FromJson(value.at("specular_color"));
    material.emissiveColor = vec3FromJson(value.at("emissive_color"));
    material.shininess = value.at("shininess").get<float>();
    material.metallic = value.at("metallic").get<float>();
    material.roughness = value.at("roughness").get<float>();
    return material;
}

Json objectToJson(const core::SceneObject& object) {
    return {
        {"name", object.name},
        {"mesh", object.mesh},
        {"material", object.material},
        {"position", vec3ToJson(object.position)},
        {"rotation_deg", vec3ToJson(object.rotationDeg)},
        {"scale", vec3ToJson(object.scale)},
        {"spinning", object.spinning}
    };
}

core::SceneObject objectFromJson(const Json& value) {
    core::SceneObject object;
    object.name = value.at("name").get<std::string>();
    object.mesh = value.at("mesh").get<std::string>();
    object.material = value.at("material").get<std::string>();
    object.position = vec3FromJson(value.at("position"));
    object.rotationDeg = vec3FromJson(value.at("rotation_deg"));
    object.scale = vec3FromJson(value.at("scale"));
    object.spinning = value.at("spinning").get<bool>();
    return object;
}

Json sceneToJson(const core::SceneDocument& scene) {
    Json materials = Json::array();
    for (const auto& material : scene.materials) {
        materials.push_back(materialToJson(material));
    }

    Json objects = Json::array();
    for (const auto& object : scene.objects) {
        objects.push_back(objectToJson(object));
    }

    return {
        {"schema_version", scene.schemaVersion},
        {"camera", {
            {"position", vec3ToJson(scene.camera.position)},
            {"yaw", scene.camera.yaw},
            {"pitch", scene.camera.pitch},
            {"fov", scene.camera.fov},
            {"movement_speed", scene.camera.movementSpeed},
            {"mouse_sensitivity", scene.camera.mouseSensitivity}
        }},
        {"lights", {
            {"directional", {
                {"direction", vec3ToJson(scene.directionalLight.direction)},
                {"color", vec3ToJson(scene.directionalLight.color)},
                {"enabled", scene.directionalLight.enabled}
            }},
            {"point", {
                {"position", vec3ToJson(scene.pointLight.position)},
                {"color", vec3ToJson(scene.pointLight.color)},
                {"constant", scene.pointLight.constant},
                {"linear", scene.pointLight.linear},
                {"quadratic", scene.pointLight.quadratic},
                {"enabled", scene.pointLight.enabled},
                {"spinning", scene.pointLight.spinning}
            }}
        }},
        {"render_settings", {
            {"post_process_effect", scene.renderSettings.postProcessEffect},
            {"shader_view_mode", scene.renderSettings.shaderViewMode},
            {"coordinate_grid", scene.renderSettings.coordinateGrid}
        }},
        {"materials", std::move(materials)},
        {"objects", std::move(objects)},
        {"selected_object", scene.selectedObject}
    };
}

core::SceneDocument sceneFromJson(const Json& value) {
    core::SceneDocument scene;
    scene.schemaVersion = value.at("schema_version").get<int>();

    const Json& camera = value.at("camera");
    scene.camera.position = vec3FromJson(camera.at("position"));
    scene.camera.yaw = camera.at("yaw").get<float>();
    scene.camera.pitch = camera.at("pitch").get<float>();
    scene.camera.fov = camera.at("fov").get<float>();
    scene.camera.movementSpeed = camera.at("movement_speed").get<float>();
    scene.camera.mouseSensitivity = camera.at("mouse_sensitivity").get<float>();

    const Json& directional = value.at("lights").at("directional");
    scene.directionalLight.direction = vec3FromJson(directional.at("direction"));
    scene.directionalLight.color = vec3FromJson(directional.at("color"));
    scene.directionalLight.enabled = directional.at("enabled").get<bool>();

    const Json& point = value.at("lights").at("point");
    scene.pointLight.position = vec3FromJson(point.at("position"));
    scene.pointLight.color = vec3FromJson(point.at("color"));
    scene.pointLight.constant = point.at("constant").get<float>();
    scene.pointLight.linear = point.at("linear").get<float>();
    scene.pointLight.quadratic = point.at("quadratic").get<float>();
    scene.pointLight.enabled = point.at("enabled").get<bool>();
    scene.pointLight.spinning = point.at("spinning").get<bool>();

    const Json& renderSettings = value.at("render_settings");
    scene.renderSettings.postProcessEffect =
        renderSettings.at("post_process_effect").get<int>();
    scene.renderSettings.shaderViewMode = renderSettings.at("shader_view_mode").get<int>();
    scene.renderSettings.coordinateGrid = renderSettings.value("coordinate_grid", true);

    const Json& materials = value.at("materials");
    if (!materials.is_array() || materials.size() > kMaxMaterials) {
        throw Json::out_of_range::create(401, "invalid materials array size", &materials);
    }
    scene.materials.reserve(materials.size());
    for (const Json& material : materials) {
        scene.materials.push_back(materialFromJson(material));
    }

    const Json& objects = value.at("objects");
    if (!objects.is_array() || objects.size() > kMaxObjects) {
        throw Json::out_of_range::create(401, "invalid objects array size", &objects);
    }
    scene.objects.reserve(objects.size());
    for (const Json& object : objects) {
        scene.objects.push_back(objectFromJson(object));
    }
    scene.selectedObject = value.at("selected_object").get<int>();
    return scene;
}

bool replaceFile(const std::filesystem::path& temporary,
                 const std::filesystem::path& destination,
                 std::string& error) {
#ifdef _WIN32
    if (MoveFileExW(temporary.c_str(), destination.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE) {
        return true;
    }
    error = "failed to replace scene file: " +
            std::error_code(static_cast<int>(GetLastError()), std::system_category()).message();
    return false;
#else
    std::error_code filesystemError;
    std::filesystem::rename(temporary, destination, filesystemError);
    if (!filesystemError) {
        return true;
    }
    error = "failed to replace scene file: " + filesystemError.message();
    return false;
#endif
}

} // namespace

namespace core {

bool validateSceneDocument(const SceneDocument& scene, std::string& error) {
    error.clear();
    if (scene.schemaVersion != kCurrentSceneSchemaVersion) {
        return setValidationError(error, "unsupported scene schema version");
    }
    if (scene.materials.size() > kMaxMaterials || scene.objects.size() > kMaxObjects) {
        return setValidationError(error, "scene exceeds supported object limits");
    }

    const SceneCamera& camera = scene.camera;
    if (!coordinatesInRange(camera.position) || !isFinite(camera.yaw) ||
        !isFinite(camera.pitch) || camera.pitch < -89.0f || camera.pitch > 89.0f ||
        !isFinite(camera.fov) || camera.fov < 1.0f || camera.fov > 120.0f ||
        !isFinite(camera.movementSpeed) || camera.movementSpeed <= 0.0f ||
        camera.movementSpeed > 1'000.0f || !isFinite(camera.mouseSensitivity) ||
        camera.mouseSensitivity <= 0.0f || camera.mouseSensitivity > 10.0f) {
        return setValidationError(error, "camera contains invalid values");
    }

    if (!coordinatesInRange(scene.directionalLight.direction) ||
        glm::dot(scene.directionalLight.direction, scene.directionalLight.direction) < 0.000001f ||
        !componentsInRange(scene.directionalLight.color, 0.0f, kMaxColorComponent)) {
        return setValidationError(error, "directional light contains invalid values");
    }

    const ScenePointLight& point = scene.pointLight;
    if (!coordinatesInRange(point.position) ||
        !componentsInRange(point.color, 0.0f, kMaxColorComponent) ||
        !isFinite(point.constant) || point.constant <= 0.0f ||
        !isFinite(point.linear) || point.linear < 0.0f ||
        !isFinite(point.quadratic) || point.quadratic < 0.0f) {
        return setValidationError(error, "point light contains invalid values");
    }

    if (scene.renderSettings.postProcessEffect < 0 ||
        scene.renderSettings.postProcessEffect > 5 ||
        scene.renderSettings.shaderViewMode < 0 ||
        scene.renderSettings.shaderViewMode > 4) {
        return setValidationError(error, "render settings are out of range");
    }

    std::unordered_set<std::string> materialIds;
    materialIds.reserve(scene.materials.size());
    for (const SceneMaterial& material : scene.materials) {
        if (!isValidText(material.id, kMaxIdLength, false) ||
            !materialIds.insert(material.id).second) {
            return setValidationError(error, "material IDs must be unique and non-empty");
        }
        if (!isSafeAssetReference(material.albedoTexture, true)) {
            return setValidationError(error, "material contains an unsafe texture reference");
        }
        if (!componentsInRange(material.albedoColor, 0.0f, kMaxColorComponent) ||
            !componentsInRange(material.specularColor, 0.0f, kMaxColorComponent) ||
            !componentsInRange(material.emissiveColor, 0.0f, kMaxColorComponent) ||
            !isFinite(material.shininess) || material.shininess < 1.0f ||
            material.shininess > 1'024.0f || !isFinite(material.metallic) ||
            material.metallic < 0.0f || material.metallic > 1.0f ||
            !isFinite(material.roughness) || material.roughness < 0.0f ||
            material.roughness > 1.0f) {
            return setValidationError(error, "material contains invalid numeric values");
        }
    }

    for (const SceneObject& object : scene.objects) {
        const bool isBuiltInMesh = object.mesh.starts_with("builtin:");
        if (!isValidText(object.name, kMaxIdLength, true) ||
            !isSafeAssetReference(object.mesh, false) ||
            (isBuiltInMesh && object.mesh != "builtin:cube") ||
            (object.mesh == "builtin:cube" && object.material.empty()) ||
            (!object.material.empty() &&
             materialIds.find(object.material) == materialIds.end())) {
            return setValidationError(error, "object contains invalid asset references");
        }
        if (!coordinatesInRange(object.position) || !coordinatesInRange(object.rotationDeg) ||
            !componentsInRange(object.scale, 0.0001f, 10'000.0f)) {
            return setValidationError(error, "object transform contains invalid values");
        }
    }

    const bool selectionIsValid = scene.objects.empty()
        ? scene.selectedObject == -1
        : scene.selectedObject >= 0 &&
          static_cast<std::size_t>(scene.selectedObject) < scene.objects.size();
    if (!selectionIsValid) {
        return setValidationError(error, "selected object index is out of range");
    }
    return true;
}

SceneIoResult saveSceneDocument(const SceneDocument& scene,
                                const std::filesystem::path& path) {
    SceneIoResult result;
    if (path.empty()) {
        result.error = "scene path is empty";
        return result;
    }
    if (!validateSceneDocument(scene, result.error)) {
        result.error = "scene validation failed: " + result.error;
        return result;
    }

    std::string serialized;
    try {
        serialized = sceneToJson(scene).dump(2);
    } catch (const std::exception& exception) {
        result.error = std::string("failed to serialize scene: ") + exception.what();
        return result;
    }
    if (serialized.size() > kMaxSceneFileSize - 1U) {
        result.error = "scene file would exceed the 32 MiB limit";
        return result;
    }

    std::error_code filesystemError;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), filesystemError);
        if (filesystemError) {
            result.error = "failed to create scene directory: " + filesystemError.message();
            return result;
        }
    }

    std::filesystem::path temporary = path;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        result.error = "failed to open temporary scene file";
        return result;
    }
    output.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
    output.put('\n');
    output.close();
    if (!output) {
        result.error = "failed to write temporary scene file";
        std::filesystem::remove(temporary, filesystemError);
        return result;
    }

    if (!replaceFile(temporary, path, result.error)) {
        std::filesystem::remove(temporary, filesystemError);
        return result;
    }
    result.success = true;
    return result;
}

SceneIoResult loadSceneDocument(const std::filesystem::path& path,
                                SceneDocument& scene) {
    SceneIoResult result;
    if (path.empty()) {
        result.error = "scene path is empty";
        return result;
    }

    std::error_code filesystemError;
    const std::uintmax_t fileSize = std::filesystem::file_size(path, filesystemError);
    if (!filesystemError && fileSize > kMaxSceneFileSize) {
        result.error = "scene file exceeds the 32 MiB limit";
        return result;
    }

    try {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            result.error = "failed to open scene file";
            return result;
        }
        const Json document = Json::parse(input, nullptr, true, true);
        SceneDocument loaded = sceneFromJson(document);
        if (!validateSceneDocument(loaded, result.error)) {
            result.error = "scene validation failed: " + result.error;
            return result;
        }
        scene = std::move(loaded);
    } catch (const std::exception& exception) {
        result.error = std::string("failed to parse scene: ") + exception.what();
        return result;
    }

    result.success = true;
    return result;
}

} // namespace core
