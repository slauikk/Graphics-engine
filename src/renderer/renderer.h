#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shader;
class Texture2D;
class Material;
class Camera;
class Mesh;

struct DirectionalLight;
struct PointLight;

class Renderer {
public:
    void init();
    void beginFrame(float r, float g, float b, float a);
    void drawMesh(const Mesh& mesh, const Material& material, const glm::mat4& model,
                  const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                  const PointLight& pointLight, int shaderViewMode);
    void endFrame();
};

#endif // RENDERER_H
