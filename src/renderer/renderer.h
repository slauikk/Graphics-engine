#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class Shader;
class Texture2D;
class Material;
class Camera;
class Mesh;

struct DirectionalLight;
struct PointLight;

struct GpuBenchmarkSample {
    float drawMs = 0.0f;
    float totalMs = 0.0f;
    float sceneMs = 0.0f;
    float uiMs = 0.0f;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void beginFrame(float r, float g, float b, float a);
    void beginBenchmarkPass();
    void endBenchmarkPass();
    void beginUiPass();
    void drawMesh(const Mesh& mesh, const Material& material, const glm::mat4& model,
                  const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                  const PointLight& pointLight, int shaderViewMode);
    void drawMeshInstanced(const Mesh& mesh, const Material& material, GLsizei instanceCount,
                           const glm::mat4& view, const glm::mat4& projection,
                           const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                           const PointLight& pointLight, int shaderIterations);
    void endFrame();
    void resetGpuFrameTimes();
    void beginGpuBenchmarkCapture(std::size_t sampleCount);
    void cancelGpuBenchmarkCapture();

    bool hasGpuFrameTime() const { return m_hasGpuFrameTime; }
    float gpuFrameTimeMs() const { return m_gpuFrameTimeMs; }
    float gpuSceneTimeMs() const { return m_gpuSceneTimeMs; }
    float gpuUiTimeMs() const { return m_gpuUiTimeMs; }
    bool hasGpuBenchmarkTime() const { return m_hasGpuBenchmarkTime; }
    float gpuBenchmarkTimeMs() const { return m_gpuBenchmarkTimeMs; }
    bool gpuBenchmarkCaptureActive() const { return m_gpuBenchmarkCaptureActive; }
    std::size_t gpuBenchmarkCaptureTarget() const { return m_gpuBenchmarkCaptureTarget; }
    const std::vector<GpuBenchmarkSample>& gpuBenchmarkCaptureSamples() const {
        return m_gpuBenchmarkCaptureSamples;
    }

private:
    static constexpr std::size_t kGpuQueryCount = 8;
    static constexpr std::size_t kTimestampsPerFrame = 5;
    static constexpr std::size_t kFrameStartTimestamp = 0;
    static constexpr std::size_t kBenchmarkStartTimestamp = 1;
    static constexpr std::size_t kBenchmarkEndTimestamp = 2;
    static constexpr std::size_t kUiStartTimestamp = 3;
    static constexpr std::size_t kFrameEndTimestamp = 4;

    void collectGpuFrameTimes();
    GLuint gpuQuery(std::size_t frameIndex, std::size_t timestampIndex) const;

    std::array<GLuint, kGpuQueryCount * kTimestampsPerFrame> m_gpuQueries{};
    std::array<std::uint64_t, kGpuQueryCount> m_gpuQueryEpochs{};
    std::array<bool, kGpuQueryCount> m_gpuBenchmarkTimestampsComplete{};
    std::array<std::uint64_t, kGpuQueryCount> m_gpuBenchmarkCaptureGenerations{};
    std::size_t m_gpuQueryWriteIndex = 0;
    std::size_t m_gpuQueryReadIndex = 0;
    std::size_t m_pendingGpuQueries = 0;
    bool m_gpuTimestampFrameActive = false;
    bool m_gpuUiTimestampIssued = false;
    bool m_gpuBenchmarkStartIssued = false;
    bool m_gpuBenchmarkEndIssued = false;
    std::uint64_t m_gpuTimingEpoch = 1;
    std::uint64_t m_gpuActiveFrameEpoch = 1;
    std::uint64_t m_gpuBenchmarkCaptureGeneration = 0;
    std::uint64_t m_gpuActiveFrameBenchmarkCaptureGeneration = 0;
    bool m_hasGpuFrameTime = false;
    float m_gpuFrameTimeMs = 0.0f;
    float m_gpuSceneTimeMs = 0.0f;
    float m_gpuUiTimeMs = 0.0f;
    bool m_hasGpuBenchmarkTime = false;
    float m_gpuBenchmarkTimeMs = 0.0f;
    std::vector<GpuBenchmarkSample> m_gpuBenchmarkCaptureSamples;
    std::size_t m_gpuBenchmarkCaptureTarget = 0;
    bool m_gpuBenchmarkCaptureActive = false;
};

#endif // RENDERER_H
