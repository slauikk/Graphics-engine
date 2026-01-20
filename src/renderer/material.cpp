#include "material.h"
#include "../shader.h"
#include "../texture2d.h"

Material::Material(Shader* shader, Texture2D* albedoTexture)
    : shader(shader), albedo(albedoTexture) {
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
    shader->setFloat("u_Shininess", shininess);
}
