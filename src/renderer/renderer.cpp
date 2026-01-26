#include "renderer.h"
#include "../shader.h"
#include "../texture2d.h"
#include "material.h"
#include "mesh/mesh.h"
#include <glad/glad.h>

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);
}

void Renderer::beginFrame(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawMesh(const Mesh& mesh, const Material& material, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos, const glm::vec3& lightPos, const glm::vec3& lightColor, bool lightEnabled) {
    material.bind();
    
    material.shader->setMat4("u_Model", model);
    material.shader->setMat4("u_View", view);
    material.shader->setMat4("u_Projection", projection);
    material.shader->setVec3("u_CameraPos", cameraPos);
    
    glm::vec3 finalLightColor = lightEnabled ? lightColor : glm::vec3(0.0f);
    material.shader->setVec3("u_LightPos", lightPos);
    material.shader->setVec3("u_LightColor", finalLightColor);
    
    mesh.draw();
}

void Renderer::endFrame() {
    // Empty for now, can be used for frame end operations
}
