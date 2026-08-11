#ifndef APPLICATION_H
#define APPLICATION_H

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>
#include "renderer/light.h"
#include "renderer/post_processor.h"
#include "editor_transform.h"
#include "editor_layout.h"
#include "scene_document.h"
#include "scene_history.h"
#include "window_settings.h"

class Camera;
class Shader;
class Texture2D;
class Renderer;
class CoordinateGrid;
class EditorChrome;
class SelectionOutline;
class Framebuffer;
class Material;
class Mesh;
class Model;

struct RenderObject {
    std::string name;
    std::string meshAsset;
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Model> model;
    glm::vec3 position;
    glm::vec3 rotationDeg;   // Euler degrees
    glm::vec3 scale;
    Material* material;      // Does not own
    bool spinning;           // Auto-rotation flag
    std::uint64_t runtimeId = 0;
};

class Application {
public:
    Application();
    ~Application();
    int run();

private:
    using MaterialCache = std::unordered_map<std::string, std::unique_ptr<Material>>;
    using TextureCache = std::unordered_map<std::string, std::shared_ptr<Texture2D>>;
    using ModelCache = std::unordered_map<std::string, std::shared_ptr<Model>>;

    enum class BenchmarkCaptureState {
        Idle,
        WarmingUp,
        Capturing,
        Complete
    };

    bool init();
    bool initBenchmarkScene();
    bool ensureSceneFramebuffer(int width, int height);
    void shutdown();
    Material* getOrCreateMaterial(const std::string& relativeTexturePath);
    Material* selectedMaterial();
    const Material* selectedMaterial() const;
    core::SceneDocument captureScene() const;
    bool applyScene(const core::SceneDocument& scene, std::string& error);
    void saveQuickScene();
    void loadQuickScene();
    std::shared_ptr<Model> loadModelAsset(
        const std::string& assetReference,
        MaterialCache& materials,
        TextureCache& textures,
        ModelCache& models,
        std::string& error);
    void loadSelectedModel(const std::string& assetReference);
    void selectObject(int direction);
    void selectObjectAtViewportPoint(core::EditorPoint point);
    void createCubeObject();
    void focusSelectedObject();
    bool transformSelectedObject(
        core::ObjectTransformCommand command,
        bool recordHistory = true);
    void duplicateSelectedObject();
    void deleteSelectedObject();
    void syncSelectedObjectToMenu();
    void checkpointScene();
    void restoreSceneHistory(bool redo);
    void resetFrameStatistics();
    void restartBenchmarkCapture();
    void updateBenchmarkCapture();
    void recordBenchmarkFrameSample(float frameIntervalMs, float cpuWorkMs, float presentMs);
    void toggleBenchmark();
    void toggleFullscreen();
    void toggleVsync();
    void persistWindowSettings();
    void setCameraInputActive(bool active);
    void processInput(float dt);
    void update(float dt);
    void render();
    void renderEditorOverlay(const core::EditorLayout& layout);
    core::EditorPoint cursorFramebufferPosition() const;
    
    // GLFW callbacks (static)
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void windowFocusCallback(GLFWwindow* window, int focused);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfwErrorCallback(int error, const char* description);
    
    // Instance callbacks
    void onFramebufferResize(int width, int height);
    void onWindowFocus(int focused);
    void onKey(int key, int action, int mods);
    void onMouseMove(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);
    
    GLFWwindow* m_window = nullptr;
    bool m_glfwInitialized = false;
    bool m_contextReady = false;
    bool m_initialized = false;
    int m_width = 1280;
    int m_height = 720;
    core::WindowSettings m_windowSettings;
    bool m_fullscreen = false;
    bool m_cameraInputActive = false;
    
    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;
    
    // FPS counter
    float m_fpsUpdateTime = 0.0f;
    int m_frameCount = 0;
    float m_currentFPS = 0.0f;
    float m_cpuFrameTimeMs = 0.0f;
    float m_cpuWorkTimeMs = 0.0f;
    float m_presentTimeMs = 0.0f;
    bool m_hasCpuWorkTime = false;
    bool m_hasPresentTime = false;
    bool m_hasFrameStatistics = false;
    bool m_skipNextFrameSample = false;
    
    // GPU info
    bool m_showGPUInfo = false;
    std::string m_gpuVendor;
    std::string m_gpuRenderer;
    std::string m_openGlVersion;
    
    // Shader reload message
    float m_reloadMessageTime = 0.0f;
    std::string m_reloadMessage;
    bool m_reloadSucceeded = false;
    float m_sceneMessageTime = 0.0f;
    std::string m_sceneMessage;
    bool m_sceneOperationSucceeded = false;
    int m_shaderViewMode = 0;
    PostProcessEffect m_postProcessEffect = PostProcessEffect::Normal;
    
    // Mouse state
    bool m_firstMouse = true;
    float m_lastX = 640.0f;
    float m_lastY = 360.0f;
    
    // Camera
    Camera* m_camera = nullptr;
    
    // Objects
    std::vector<RenderObject> m_objects;
    int m_selectedObject = 0;
    int m_activeTransformKey = GLFW_KEY_UNKNOWN;
    std::uint64_t m_activeTransformObjectId = 0;
    std::uint64_t m_nextObjectId = 1;
    core::SceneHistory m_sceneHistory;
    
    // Rendering resources
    std::shared_ptr<Mesh> m_cubeMesh = nullptr;
    std::shared_ptr<Shader> m_shader = nullptr;
    Renderer* m_renderer = nullptr;
    std::unique_ptr<CoordinateGrid> m_coordinateGrid;
    std::unique_ptr<EditorChrome> m_editorChrome;
    std::unique_ptr<SelectionOutline> m_selectionOutline;
    bool m_showCoordinateGrid = true;
    std::unique_ptr<Framebuffer> m_sceneFramebuffer;
    std::unique_ptr<Framebuffer> m_benchmarkFramebuffer;
    std::unique_ptr<PostProcessor> m_postProcessor;

    // Fixed, single-draw GPU benchmark resources.
    GLsizei m_benchmarkInstanceCount = 0;
    std::unique_ptr<Material> m_benchmarkMaterial;
    bool m_benchmarkEnabled = false;
    BenchmarkCaptureState m_benchmarkCaptureState = BenchmarkCaptureState::Idle;
    double m_benchmarkWarmupStart = 0.0;
    float m_benchmarkMedianGpuMs = 0.0f;
    float m_benchmarkP95GpuMs = 0.0f;
    std::string m_benchmarkReportStatus;
    std::string m_benchmarkComparisonStatus;
    bool m_benchmarkCpuCaptureActive = false;
    std::vector<float> m_benchmarkFrameIntervalSamples;
    std::vector<float> m_benchmarkCpuWorkSamples;
    std::vector<float> m_benchmarkPresentSamples;
    
    // Cached ownership; RenderObject keeps non-owning Material pointers.
    MaterialCache m_materials;
    TextureCache m_textures;
    ModelCache m_models;
    
    // Lighting
    DirectionalLight m_dirLight;
    PointLight m_pointLight;
    bool m_pointLightSpinning = false;
    float m_pointLightSpinAngle = 0.0f;
    
    // Light sphere visualization
    GLuint m_lightVAO = 0;
    GLuint m_lightVBO = 0;
    GLuint m_lightEBO = 0;
    int m_lightIndexCount = 0;
    Shader* m_lightShader = nullptr;
    
};

#endif // APPLICATION_H
