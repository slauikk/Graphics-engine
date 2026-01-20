#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;
class Texture2D;

class Material {
public:
    Material(Shader* shader, Texture2D* albedoTexture = nullptr);
    
    void bind() const;
    
    glm::vec3 albedoColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;
    glm::vec3 specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
    
    Texture2D* albedo = nullptr;
    Shader* shader = nullptr;
};

#endif // MATERIAL_H
