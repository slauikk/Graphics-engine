#include "renderer.h"
#include "../shader.h"
#include "../texture2d.h"
#include "material.h"
#include "mesh/mesh.h"
#include "light.h"
#include <glad/glad.h>

namespace {

bool prepareMeshDraw(const Material& material, const glm::mat4& model,
                     const glm::mat4& view, const glm::mat4& projection,
                     const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                     const PointLight& pointLight, int shaderViewMode,
                     bool useInstancing, int fragmentWorkIterations) {
    if (material.shader == nullptr || material.shader->m_id == 0) {
        return false;
    }

    material.bind();
    material.shader->setMat4("u_Model", model);
    material.shader->setMat4("u_View", view);
    material.shader->setMat4("u_Projection", projection);
    material.shader->setVec3("u_CameraPos", cameraPos);
    material.shader->setInt("u_DebugViewMode", shaderViewMode);
    material.shader->setInt("u_UseInstancing", useInstancing ? 1 : 0);
    material.shader->setInt("u_BenchmarkIterations", fragmentWorkIterations);

    material.shader->setVec3("u_DirLight.direction", dirLight.direction);
    material.shader->setVec3("u_DirLight.color", dirLight.color);
    material.shader->setInt("u_DirLight.enabled", dirLight.enabled ? 1 : 0);

    material.shader->setVec3("u_PointLight.position", pointLight.position);
    material.shader->setVec3("u_PointLight.color", pointLight.color);
    material.shader->setFloat("u_PointLight.constant", pointLight.constant);
    material.shader->setFloat("u_PointLight.linear", pointLight.linear);
    material.shader->setFloat("u_PointLight.quadratic", pointLight.quadratic);
    material.shader->setInt("u_PointLight.enabled", pointLight.enabled ? 1 : 0);
    return true;
}

} // namespace

Renderer::~Renderer() {
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

    if (!m_gpuTimestampFrameActive && m_pendingGpuQueries < kGpuQueryCount) {
        const GLuint query = gpuQuery(m_gpuQueryWriteIndex, kFrameStartTimestamp);
        if (query != 0) {
            glQueryCounter(query, GL_TIMESTAMP);
            m_gpuTimestampFrameActive = true;
            m_gpuUiTimestampIssued = false;
            m_gpuBenchmarkStartIssued = false;
            m_gpuBenchmarkEndIssued = false;
            m_gpuActiveFrameEpoch = m_gpuTimingEpoch;
            m_gpuActiveFrameBenchmarkCaptureGeneration = 0;
        }
    }

    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::beginBenchmarkPass() {
    if (!m_gpuTimestampFrameActive || m_gpuBenchmarkStartIssued) {
        return;
    }

    glQueryCounter(gpuQuery(m_gpuQueryWriteIndex, kBenchmarkStartTimestamp), GL_TIMESTAMP);
    m_gpuBenchmarkStartIssued = true;
    m_gpuActiveFrameBenchmarkCaptureGeneration =
        m_gpuBenchmarkCaptureActive ? m_gpuBenchmarkCaptureGeneration : 0;
}

void Renderer::endBenchmarkPass() {
    if (!m_gpuTimestampFrameActive || !m_gpuBenchmarkStartIssued || m_gpuBenchmarkEndIssued) {
        return;
    }

    glQueryCounter(gpuQuery(m_gpuQueryWriteIndex, kBenchmarkEndTimestamp), GL_TIMESTAMP);
    m_gpuBenchmarkEndIssued = true;
}

void Renderer::beginUiPass() {
    if (!m_gpuTimestampFrameActive || m_gpuUiTimestampIssued) {
        return;
    }

    endBenchmarkPass();
    glQueryCounter(gpuQuery(m_gpuQueryWriteIndex, kUiStartTimestamp), GL_TIMESTAMP);
    m_gpuUiTimestampIssued = true;
}

void Renderer::drawMesh(const Mesh& mesh, const Material& material, const glm::mat4& model,
                        const glm::mat4& view, const glm::mat4& projection,
                        const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                        const PointLight& pointLight, int shaderViewMode) {
    if (!prepareMeshDraw(material, model, view, projection, cameraPos,
                         dirLight, pointLight, shaderViewMode, false, 0)) {
        return;
    }
    mesh.draw();
}

void Renderer::drawMeshInstanced(const Mesh& mesh, const Material& material, GLsizei instanceCount,
                                 const glm::mat4& view, const glm::mat4& projection,
                                 const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                                 const PointLight& pointLight, int shaderIterations) {
    if (instanceCount <= 0 ||
        !prepareMeshDraw(material, glm::mat4(1.0f), view, projection, cameraPos,
                         dirLight, pointLight, 0, true, shaderIterations)) {
        return;
    }
    mesh.drawInstanced(instanceCount);
}

void Renderer::endFrame() {
    if (!m_gpuTimestampFrameActive) {
        return;
    }

    if (!m_gpuUiTimestampIssued) {
        beginUiPass();
    }
    glQueryCounter(gpuQuery(m_gpuQueryWriteIndex, kFrameEndTimestamp), GL_TIMESTAMP);
    m_gpuQueryEpochs[m_gpuQueryWriteIndex] = m_gpuActiveFrameEpoch;
    m_gpuBenchmarkTimestampsComplete[m_gpuQueryWriteIndex] =
        m_gpuBenchmarkStartIssued && m_gpuBenchmarkEndIssued;
    m_gpuBenchmarkCaptureGenerations[m_gpuQueryWriteIndex] =
        m_gpuBenchmarkTimestampsComplete[m_gpuQueryWriteIndex]
            ? m_gpuActiveFrameBenchmarkCaptureGeneration
            : 0;

    m_gpuTimestampFrameActive = false;
    m_gpuUiTimestampIssued = false;
    m_gpuBenchmarkStartIssued = false;
    m_gpuBenchmarkEndIssued = false;
    m_gpuActiveFrameBenchmarkCaptureGeneration = 0;
    m_gpuQueryWriteIndex = (m_gpuQueryWriteIndex + 1) % kGpuQueryCount;
    ++m_pendingGpuQueries;
}

void Renderer::resetGpuFrameTimes() {
    ++m_gpuTimingEpoch;
    cancelGpuBenchmarkCapture();
    m_hasGpuFrameTime = false;
    m_gpuFrameTimeMs = 0.0f;
    m_gpuSceneTimeMs = 0.0f;
    m_gpuUiTimeMs = 0.0f;
    m_hasGpuBenchmarkTime = false;
    m_gpuBenchmarkTimeMs = 0.0f;
}

void Renderer::beginGpuBenchmarkCapture(std::size_t sampleCount) {
    m_gpuBenchmarkCaptureSamples.clear();
    m_gpuBenchmarkCaptureTarget = sampleCount;
    m_gpuBenchmarkCaptureSamples.reserve(sampleCount);
    m_gpuBenchmarkCaptureActive = false;

    if (sampleCount > 0) {
        ++m_gpuBenchmarkCaptureGeneration;
        if (m_gpuBenchmarkCaptureGeneration == 0) {
            ++m_gpuBenchmarkCaptureGeneration;
        }
        m_gpuBenchmarkCaptureActive = true;
    }
}

void Renderer::cancelGpuBenchmarkCapture() {
    m_gpuBenchmarkCaptureActive = false;
    m_gpuBenchmarkCaptureTarget = 0;
    m_gpuBenchmarkCaptureSamples.clear();
}

void Renderer::collectGpuFrameTimes() {
    while (m_pendingGpuQueries > 0) {
        const GLuint endQuery = gpuQuery(m_gpuQueryReadIndex, kFrameEndTimestamp);
        GLuint available = GL_FALSE;
        glGetQueryObjectuiv(endQuery, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available == GL_FALSE) {
            break;
        }

        GLuint64 frameStart = 0;
        GLuint64 uiStart = 0;
        GLuint64 frameEnd = 0;
        glGetQueryObjectui64v(gpuQuery(m_gpuQueryReadIndex, kFrameStartTimestamp),
                              GL_QUERY_RESULT, &frameStart);
        glGetQueryObjectui64v(gpuQuery(m_gpuQueryReadIndex, kUiStartTimestamp),
                              GL_QUERY_RESULT, &uiStart);
        glGetQueryObjectui64v(endQuery, GL_QUERY_RESULT, &frameEnd);

        if (m_gpuQueryEpochs[m_gpuQueryReadIndex] == m_gpuTimingEpoch &&
            frameStart <= uiStart && uiStart <= frameEnd) {
            constexpr float nanosecondsToMilliseconds = 1.0f / 1'000'000.0f;
            const float frameSample = static_cast<float>(frameEnd - frameStart) * nanosecondsToMilliseconds;
            const float sceneSample = static_cast<float>(uiStart - frameStart) * nanosecondsToMilliseconds;
            const float uiSample = static_cast<float>(frameEnd - uiStart) * nanosecondsToMilliseconds;

            if (m_gpuBenchmarkTimestampsComplete[m_gpuQueryReadIndex]) {
                GLuint64 benchmarkStart = 0;
                GLuint64 benchmarkEnd = 0;
                glGetQueryObjectui64v(gpuQuery(m_gpuQueryReadIndex, kBenchmarkStartTimestamp),
                                      GL_QUERY_RESULT, &benchmarkStart);
                glGetQueryObjectui64v(gpuQuery(m_gpuQueryReadIndex, kBenchmarkEndTimestamp),
                                      GL_QUERY_RESULT, &benchmarkEnd);

                if (frameStart <= benchmarkStart && benchmarkStart <= benchmarkEnd &&
                    benchmarkEnd <= uiStart) {
                    const float benchmarkSample =
                        static_cast<float>(benchmarkEnd - benchmarkStart) * nanosecondsToMilliseconds;

                    if (m_hasGpuBenchmarkTime) {
                        constexpr float smoothingFactor = 0.1f;
                        m_gpuBenchmarkTimeMs +=
                            (benchmarkSample - m_gpuBenchmarkTimeMs) * smoothingFactor;
                    } else {
                        m_gpuBenchmarkTimeMs = benchmarkSample;
                        m_hasGpuBenchmarkTime = true;
                    }

                    if (m_gpuBenchmarkCaptureActive &&
                        m_gpuBenchmarkCaptureGenerations[m_gpuQueryReadIndex] ==
                            m_gpuBenchmarkCaptureGeneration) {
                        m_gpuBenchmarkCaptureSamples.push_back({
                            benchmarkSample, frameSample, sceneSample, uiSample
                        });
                        if (m_gpuBenchmarkCaptureSamples.size() >= m_gpuBenchmarkCaptureTarget) {
                            m_gpuBenchmarkCaptureActive = false;
                        }
                    }
                }
            }

            if (m_hasGpuFrameTime) {
                constexpr float smoothingFactor = 0.1f;
                m_gpuFrameTimeMs += (frameSample - m_gpuFrameTimeMs) * smoothingFactor;
                m_gpuSceneTimeMs += (sceneSample - m_gpuSceneTimeMs) * smoothingFactor;
                m_gpuUiTimeMs += (uiSample - m_gpuUiTimeMs) * smoothingFactor;
            } else {
                m_gpuFrameTimeMs = frameSample;
                m_gpuSceneTimeMs = sceneSample;
                m_gpuUiTimeMs = uiSample;
                m_hasGpuFrameTime = true;
            }
        }

        m_gpuBenchmarkTimestampsComplete[m_gpuQueryReadIndex] = false;
        m_gpuBenchmarkCaptureGenerations[m_gpuQueryReadIndex] = 0;
        m_gpuQueryReadIndex = (m_gpuQueryReadIndex + 1) % kGpuQueryCount;
        --m_pendingGpuQueries;
    }
}

GLuint Renderer::gpuQuery(std::size_t frameIndex, std::size_t timestampIndex) const {
    return m_gpuQueries[frameIndex * kTimestampsPerFrame + timestampIndex];
}
