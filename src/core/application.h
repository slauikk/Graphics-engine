#ifndef APPLICATION_H
#define APPLICATION_H

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>
#include "renderer/light.h"

class Camera;
class Shader;
class Texture2D;
class Renderer;
class Material;
class Mesh;

struct RenderObject {
    glm::vec3 position;
    glm::vec3 rotationDeg;   // Euler degrees
    glm::vec3 scale;
    Material* material;      // Does not own
    bool spinning;           // Auto-rotation flag
};

class Application {
public:
    Application();
    ~Application();
    int run();

private:
    bool init();
    void shutdown();
    void applyGlobalShininess();
    Material* getOrCreateMaterial(const std::string& relativeTexturePath);
    void processInput(float dt);
    void update(float dt);
    void render();
    
    // GLFW callbacks (static)
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void glfwErrorCallback(int error, const char* description);
    
    // Instance callbacks
    void onFramebufferResize(int width, int height);
    void onMouseMove(double xpos, double ypos);
    void onScroll(double xoffset, double yoffset);
    
    GLFWwindow* m_window = nullptr;
    bool m_glfwInitialized = false;
    bool m_contextReady = false;
    bool m_initialized = false;
    int m_width = 1280;
    int m_height = 720;
    
    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;
    
    // FPS counter
    float m_fpsUpdateTime = 0.0f;
    int m_frameCount = 0;
    float m_currentFPS = 0.0f;
    
    // GPU info
    bool m_showGPUInfo = false;
    std::string m_gpuRenderer;
    
    // Key debounce flags
    bool m_f9Pressed = false;
    bool m_f8Pressed = false;
    bool m_f5Pressed = false;
    bool m_upPressed = false;
    bool m_downPressed = false;
    bool m_enterPressed = false;
    bool m_escPressed = false;
    
    // Shader reload message
    float m_reloadMessageTime = 0.0f;
    std::string m_reloadMessage;
    bool m_reloadSucceeded = false;
    int m_shaderViewMode = 0;
    
    // Mouse state
    bool m_firstMouse = true;
    float m_lastX = 640.0f;
    float m_lastY = 360.0f;
    
    // Camera
    Camera* m_camera = nullptr;
    
    // Objects
    std::vector<RenderObject> m_objects;
    int m_selectedObject = 0;
    
    // Rendering resources
    std::shared_ptr<Mesh> m_cubeMesh = nullptr;
    std::shared_ptr<Shader> m_shader = nullptr;
    Renderer* m_renderer = nullptr;
    
    // Cached ownership; RenderObject keeps non-owning Material pointers.
    std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_textures;
    
    // Lighting
    DirectionalLight m_dirLight;
    PointLight m_pointLight;
    float m_shininess = 32.0f;
    bool m_pointLightSpinning = false;
    float m_pointLightSpinAngle = 0.0f;
    
    // Light sphere visualization
    GLuint m_lightVAO = 0;
    GLuint m_lightVBO = 0;
    GLuint m_lightEBO = 0;
    int m_lightIndexCount = 0;
    Shader* m_lightShader = nullptr;
    
    // Key debounce for lighting controls
    bool m_lPressed = false;
    bool m_oPressed = false;
    bool m_jPressed = false;
    bool m_kPressed = false;
};

#endif // APPLICATION_H
