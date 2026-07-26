#include "post_processor.h"
#include "../core/resource_manager.h"
#include "../shader.h"

namespace {

class ScopedPostProcessState {
public:
    ScopedPostProcessState()
        : m_depthTestEnabled(glIsEnabled(GL_DEPTH_TEST)),
          m_blendEnabled(glIsEnabled(GL_BLEND)),
          m_cullFaceEnabled(glIsEnabled(GL_CULL_FACE)) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &m_activeTexture);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &m_depthWriteEnabled);

        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_texture2D);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
    }

    ~ScopedPostProcessState() {
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_texture2D));
        glActiveTexture(static_cast<GLenum>(m_activeTexture));
        glDepthMask(m_depthWriteEnabled);

        restoreCapability(GL_DEPTH_TEST, m_depthTestEnabled);
        restoreCapability(GL_BLEND, m_blendEnabled);
        restoreCapability(GL_CULL_FACE, m_cullFaceEnabled);
    }

private:
    static void restoreCapability(GLenum capability, GLboolean enabled) {
        if (enabled == GL_TRUE) {
            glEnable(capability);
        } else {
            glDisable(capability);
        }
    }

    GLboolean m_depthTestEnabled = GL_FALSE;
    GLboolean m_depthWriteEnabled = GL_TRUE;
    GLboolean m_blendEnabled = GL_FALSE;
    GLboolean m_cullFaceEnabled = GL_FALSE;
    GLint m_program = 0;
    GLint m_vertexArray = 0;
    GLint m_activeTexture = GL_TEXTURE0;
    GLint m_texture2D = 0;
};

} // namespace

PostProcessor::~PostProcessor() {
    if (m_vertexArray != 0) {
        glDeleteVertexArrays(1, &m_vertexArray);
    }
}

bool PostProcessor::init() {
    m_shader = ResourceManager::getShader(
        "shaders/post_process.vert", "shaders/post_process.frag");
    if (!m_shader || m_shader->m_id == 0) {
        return false;
    }

    glGenVertexArrays(1, &m_vertexArray);
    return m_vertexArray != 0;
}

void PostProcessor::render(GLuint sceneTexture, PostProcessEffect effect,
                           float elapsedSeconds) const {
    if (!isReady() || sceneTexture == 0) {
        return;
    }

    const ScopedPostProcessState restoreState;
    m_shader->use();
    m_shader->setInt("u_Scene", 0);
    m_shader->setInt("u_Effect", static_cast<int>(effect));
    m_shader->setFloat("u_Time", elapsedSeconds);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

bool PostProcessor::isReady() const {
    return m_vertexArray != 0 && m_shader && m_shader->m_id != 0;
}

const char* PostProcessor::effectName(PostProcessEffect effect) {
    switch (effect) {
        case PostProcessEffect::Grayscale: return "Grayscale";
        case PostProcessEffect::Invert: return "Invert";
        case PostProcessEffect::Edges: return "Edges";
        case PostProcessEffect::Vignette: return "Vignette";
        case PostProcessEffect::Crt: return "CRT";
        case PostProcessEffect::Normal:
        default: return "Normal";
    }
}
