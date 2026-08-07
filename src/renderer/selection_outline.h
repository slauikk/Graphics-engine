#ifndef SELECTION_OUTLINE_H
#define SELECTION_OUTLINE_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>

class Mesh;
class Shader;

class SelectionOutline {
public:
    bool init();
    bool beginMask();
    void maskObject(bool selected) const;
    void beginOutline();
    void drawMesh(const Mesh& mesh, const glm::mat4& model,
                  const glm::mat4& view, const glm::mat4& projection) const;
    void end();

    bool isReady() const;

private:
    struct StencilFaceState {
        GLint function = GL_ALWAYS;
        GLint reference = 0;
        GLint valueMask = -1;
        GLint writeMask = -1;
        GLint stencilFail = GL_KEEP;
        GLint depthFail = GL_KEEP;
        GLint depthPass = GL_KEEP;
    };

    static StencilFaceState captureFaceState(bool backFace);
    static void restoreFaceState(GLenum face, const StencilFaceState& state);

    std::shared_ptr<Shader> m_shader;
    StencilFaceState m_frontState;
    StencilFaceState m_backState;
    GLboolean m_stencilWasEnabled = GL_FALSE;
    GLboolean m_depthWriteMask = GL_TRUE;
    GLint m_stencilClearValue = 0;
    bool m_passActive = false;
    bool m_outlineActive = false;
};

#endif // SELECTION_OUTLINE_H
