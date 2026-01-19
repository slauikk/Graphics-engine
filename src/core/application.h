#ifndef APPLICATION_H
#define APPLICATION_H

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <sstream>

class Camera;
class Shader;
class Texture2D;
class Renderer;

class Application {
public:
    Application();
    ~Application();
    int run();

private:
    void init();
    void shutdown();
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
    const char* m_gpuRenderer = nullptr;
    
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
    
    // Mouse state
    bool m_firstMouse = true;
    float m_lastX = 640.0f;
    float m_lastY = 360.0f;
    
    // Camera
    Camera* m_camera = nullptr;
    
    // Cube rotation state
    float m_rotationX = 0.0f;
    float m_rotationY = 0.0f;
    float m_rotationZ = 0.0f;
    bool m_isSpinning = false;
    
    // Rendering resources
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    Shader* m_shader = nullptr;
    Texture2D* m_texture = nullptr;
    Renderer* m_renderer = nullptr;
};

#endif // APPLICATION_H
