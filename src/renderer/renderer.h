#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shader;
class Texture2D;

class Renderer {
public:
    void init();
    void beginFrame(float r, float g, float b, float a);
    void drawTexturedCube(const Shader& shader, GLuint vao, const Texture2D& tex, const glm::mat4& mvp);
    void endFrame();
};

#endif // RENDERER_H
