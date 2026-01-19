#include "renderer.h"
#include "../shader.h"
#include "../texture2d.h"
#include <glad/glad.h>

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);
}

void Renderer::beginFrame(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawTexturedCube(const Shader& shader, GLuint vao, const Texture2D& tex, const glm::mat4& mvp) {
    shader.use();
    shader.setInt("u_Texture", 0);
    tex.bind(0);
    shader.setMat4("u_MVP", mvp);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::endFrame() {
    // Empty for now, can be used for frame end operations
}
