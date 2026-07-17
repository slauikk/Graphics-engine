#include "renderer.h"
#include "../shader.h"
#include "../texture2d.h"
#include "material.h"
#include "mesh/mesh.h"
#include "light.h"
#include <glad/glad.h>

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);
}

void Renderer::beginFrame(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawMesh(const Mesh& mesh, const Material& material, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, const DirectionalLight& dirLight, const PointLight& pointLight) {
    if (material.shader == nullptr || material.shader->m_id == 0) {
        return;
    }

    material.bind();
    
    material.shader->setMat4("u_Model", model);
    material.shader->setMat4("u_View", view);
    material.shader->setMat4("u_Projection", projection);
    material.shader->setVec3("u_CameraPos", cameraPos);
    
    // Directional Light uniforms
    material.shader->setVec3("u_DirLight.direction", dirLight.direction);
    material.shader->setVec3("u_DirLight.color", dirLight.color);
    material.shader->setInt("u_DirLight.enabled", dirLight.enabled ? 1 : 0);
    
    // Point Light uniforms
    material.shader->setVec3("u_PointLight.position", pointLight.position);
    material.shader->setVec3("u_PointLight.color", pointLight.color);
    material.shader->setFloat("u_PointLight.constant", pointLight.constant);
    material.shader->setFloat("u_PointLight.linear", pointLight.linear);
    material.shader->setFloat("u_PointLight.quadratic", pointLight.quadratic);
    material.shader->setInt("u_PointLight.enabled", pointLight.enabled ? 1 : 0);
    
    mesh.draw();
}

void Renderer::endFrame() {
    // Empty for now, can be used for frame end operations
}
