#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

class Model;

struct ImportedMaterial {
    std::string id;
    std::string name;
    std::string albedoTexture;
    glm::vec3 albedoColor = glm::vec3(1.0f);
    glm::vec3 specularColor = glm::vec3(0.5f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float shininess = 32.0f;
    float metallic = 0.0f;
    float roughness = 0.0f;
};

struct ModelLoadResult {
    std::shared_ptr<Model> model;
    std::vector<ImportedMaterial> materials;
    std::vector<std::string> warnings;
    std::string error;

    bool success() const { return model != nullptr && error.empty(); }
};

namespace ModelLoader {

ModelLoadResult load(const std::string& assetReference);

} // namespace ModelLoader

#endif // MODEL_LOADER_H
