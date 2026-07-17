#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cstddef>

class Shader;
class Texture2D;
class Material;
class Camera;
class Mesh;

struct DirectionalLight;
struct PointLight;

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void beginFrame(float r, float g, float b, float a);
    void beginUiPass();
    void drawMesh(const Mesh& mesh, const Material& material, const glm::mat4& model,
                  const glm::mat4& view, const glm::mat4& projection,
                  const glm::vec3& cameraPos, const DirectionalLight& dirLight,
                  const PointLight& pointLight, int shaderViewMode);
    void endFrame();

    bool hasGpuFrameTime() const { return m_hasGpuFrameTime; }
    float gpuFrameTimeMs() const { return m_gpuFrameTimeMs; }
    float gpuSceneTimeMs() const { return m_gpuSceneTimeMs; }
    float gpuUiTimeMs() const { return m_gpuUiTimeMs; }

private:
    static constexpr std::size_t kGpuQueryCount = 8;
    static constexpr std::size_t kTimestampsPerFrame = 3;
    static constexpr std::size_t kFrameStartTimestamp = 0;
    static constexpr std::size_t kUiStartTimestamp = 1;
    static constexpr std::size_t kFrameEndTimestamp = 2;

    void collectGpuFrameTimes();
    GLuint gpuQuery(std::size_t frameIndex, std::size_t timestampIndex) const;

    std::array<GLuint, kGpuQueryCount * kTimestampsPerFrame> m_gpuQueries{};
    std::size_t m_gpuQueryWriteIndex = 0;
    std::size_t m_gpuQueryReadIndex = 0;
    std::size_t m_pendingGpuQueries = 0;
    bool m_gpuTimestampFrameActive = false;
    bool m_gpuUiTimestampIssued = false;
    bool m_hasGpuFrameTime = false;
    float m_gpuFrameTimeMs = 0.0f;
    float m_gpuSceneTimeMs = 0.0f;
    float m_gpuUiTimeMs = 0.0f;
};

#endif // RENDERER_H
