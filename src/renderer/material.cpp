#include "material.h"
#include "../shader.h"
#include "../texture2d.h"

#include <utility>

Material::Material(Shader* shader, Texture2D* albedoTexture,
                   std::string materialId, std::string texturePath)
    : id(std::move(materialId))
    , albedoTexturePath(std::move(texturePath))
    , albedo(albedoTexture)
    , shader(shader) {
}

void Material::bind() const {
    if (!shader) {
        return;
    }
    
    shader->use();
    
    if (albedo != nullptr) {
        albedo->bind(0);
        shader->setInt("u_AlbedoTex", 0);
        shader->setInt("u_UseAlbedoTex", 1);
    } else {
        shader->setInt("u_UseAlbedoTex", 0);
    }
    
    shader->setVec3("u_AlbedoColor", albedoColor);
    shader->setVec3("u_SpecularColor", specularColor);
    shader->setVec3("u_EmissiveColor", emissiveColor);
    shader->setFloat("u_Shininess", shininess);
    shader->setFloat("u_Metallic", metallic);
    shader->setFloat("u_Roughness", roughness);
}
