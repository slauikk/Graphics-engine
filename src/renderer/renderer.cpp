#include "renderer.h"
#include "../shader.h"
#include "../texture2d.h"
#include "material.h"
#include "mesh/mesh.h"
#include "light.h"
#include <glad/glad.h>

Renderer::~Renderer() {
    if (m_gpuQueryActive) {
        glEndQuery(GL_TIME_ELAPSED);
        m_gpuQueryActive = false;
    }

    if (m_gpuQueries.front() != 0) {
        glDeleteQueries(static_cast<GLsizei>(m_gpuQueries.size()), m_gpuQueries.data());
        m_gpuQueries.fill(0);
    }
}

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);

    if (m_gpuQueries.front() == 0) {
        glGenQueries(static_cast<GLsizei>(m_gpuQueries.size()), m_gpuQueries.data());
    }
}

void Renderer::beginFrame(float r, float g, float b, float a) {
    collectGpuFrameTimes();

    if (!m_gpuQueryActive && m_pendingGpuQueries < m_gpuQueries.size()) {
        const GLuint query = m_gpuQueries[m_gpuQueryWriteIndex];
        if (query != 0) {
            glBeginQuery(GL_TIME_ELAPSED, query);
            m_gpuQueryActive = true;
        }
    }

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawMesh(const Mesh& mesh, const Material& material, const glm::mat4& model,
                        const glm::mat4& view, const glm::mat4& projection,
                        const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                        const PointLight& pointLight, int shaderViewMode) {
    if (material.shader == nullptr || material.shader->m_id == 0) {
        return;
    }

    material.bind();
    
    material.shader->setMat4("u_Model", model);
    material.shader->setMat4("u_View", view);
    material.shader->setMat4("u_Projection", projection);
    material.shader->setVec3("u_CameraPos", cameraPos);
    material.shader->setInt("u_DebugViewMode", shaderViewMode);
    
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
    if (!m_gpuQueryActive) {
        return;
    }

    glEndQuery(GL_TIME_ELAPSED);
    m_gpuQueryActive = false;
    m_gpuQueryWriteIndex = (m_gpuQueryWriteIndex + 1) % m_gpuQueries.size();
    ++m_pendingGpuQueries;
}

void Renderer::collectGpuFrameTimes() {
    while (m_pendingGpuQueries > 0) {
        const GLuint query = m_gpuQueries[m_gpuQueryReadIndex];
        GLuint available = GL_FALSE;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available == GL_FALSE) {
            break;
        }

        GLuint64 elapsedNanoseconds = 0;
        glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsedNanoseconds);
        const float sampleMs = static_cast<float>(elapsedNanoseconds) / 1'000'000.0f;

        if (m_hasGpuFrameTime) {
            constexpr float smoothingFactor = 0.1f;
            m_gpuFrameTimeMs += (sampleMs - m_gpuFrameTimeMs) * smoothingFactor;
        } else {
            m_gpuFrameTimeMs = sampleMs;
            m_hasGpuFrameTime = true;
        }

        m_gpuQueryReadIndex = (m_gpuQueryReadIndex + 1) % m_gpuQueries.size();
        --m_pendingGpuQueries;
    }
}
