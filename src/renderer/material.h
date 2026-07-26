#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader;
class Texture2D;

class Material {
public:
    Material(Shader* shader, Texture2D* albedoTexture = nullptr,
             std::string materialId = {}, std::string albedoTexturePath = {});
    
    void bind() const;
    
    glm::vec3 albedoColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;
    glm::vec3 specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float metallic = 0.0f;
    float roughness = 0.0f;

    std::string id;
    std::string albedoTexturePath;
    
    Texture2D* albedo = nullptr;
    Shader* shader = nullptr;
};

#endif // MATERIAL_H
