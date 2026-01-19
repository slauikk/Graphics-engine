#include "application.h"
#include "../camera.h"
#include "../shader.h"
#include "../texture2d.h"
#include "../ui_text.h"
#include "../menu.h"
#include "../gl_debug.h"
#include "../renderer/renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

Application::Application() {
    init();
}

Application::~Application() {
    shutdown();
}

void Application::init() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    m_window = glfwCreateWindow(mode->width, mode->height, "Graphics_engine", monitor, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // VSync 1 - on 0 - off
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // Enable debug output
    GL_EnableDebugOutput();

    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

    // UI text
    UIText::init(m_width, m_height);

    // Camera
    m_camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));

    // Mouse callbacks
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Shaders - try from files first, fallback to inline
    const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 u_MVP;

void main() {
    gl_Position = u_MVP * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

    const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_Texture;

void main() {
    FragColor = texture(u_Texture, TexCoord);
}
)";

    m_shader = new Shader("assets/shaders/textured.vert", "assets/shaders/textured.frag");
    if (m_shader->m_id == 0) {
        std::cerr << "[Application] Shader files not found, using inline fallback\n";
        delete m_shader;
        m_shader = new Shader(vertexShaderSource, fragmentShaderSource);
        if (m_shader->m_id == 0) {
            std::cerr << "Failed to create shader program\n";
            return;
        }
    }

    // Cube with UV coordinates (position.xyz + texcoord.xy)
    float vertices[] = {
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        // Left face
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        // Right face
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    m_texture = new Texture2D();
    m_texture->loadGeneratedGrid();
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_gpuRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    
    // Renderer
    m_renderer = new Renderer();
    m_renderer->init();
    
    // Texture menu
    Menu::init();
}

void Application::shutdown() {
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
    }
    if (m_VBO != 0) {
        glDeleteBuffers(1, &m_VBO);
    }
    if (m_shader) {
        delete m_shader;
    }
    if (m_texture) {
        delete m_texture;
    }
    if (m_camera) {
        delete m_camera;
    }
    if (m_renderer) {
        delete m_renderer;
    }
    UIText::cleanup();
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

int Application::run() {
    if (!m_window) {
        return 1;
    }

    while (!glfwWindowShouldClose(m_window)) {
        // Delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        m_deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        processInput(m_deltaTime);
        update(m_deltaTime);
        render();
        
        m_renderer->endFrame();
        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    return 0;
}

void Application::processInput(float dt) {
    // FPS
    m_frameCount++;
    m_fpsUpdateTime += dt;
    if (m_fpsUpdateTime >= 0.5f) {
        m_currentFPS = m_frameCount / m_fpsUpdateTime;
        m_frameCount = 0;
        m_fpsUpdateTime = 0.0f;
    }

    // F9 — toggle GPU info
    if (glfwGetKey(m_window, GLFW_KEY_F9) == GLFW_PRESS && !m_f9Pressed) {
        m_showGPUInfo = !m_showGPUInfo;
        m_f9Pressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_F9) == GLFW_RELEASE) {
        m_f9Pressed = false;
    }
    
    // F5 — reload shaders
    if (glfwGetKey(m_window, GLFW_KEY_F5) == GLFW_PRESS && !m_f5Pressed) {
        bool mainReload = false;
        bool uiReload = false;
        
        if (m_shader != nullptr) {
            mainReload = m_shader->reload();
        }
        uiReload = UIText::reloadShaders();
        
        if (mainReload && uiReload) {
            m_reloadMessage = "Shaders reloaded";
            m_reloadMessageTime = 2.0f;
        } else {
            m_reloadMessage = "Reload failed";
            m_reloadMessageTime = 2.0f;
        }
        m_f5Pressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_F5) == GLFW_RELEASE) {
        m_f5Pressed = false;
    }
    
    // F8 — toggle texture menu
    if (glfwGetKey(m_window, GLFW_KEY_F8) == GLFW_PRESS && !m_f8Pressed) {
        Menu::toggle();
        m_f8Pressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_F8) == GLFW_RELEASE) {
        m_f8Pressed = false;
    }
    
    if (Menu::isOpen()) {
        if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS && !m_upPressed) {
            Menu::processKey(GLFW_KEY_UP);
            m_upPressed = true;
        }
        if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_RELEASE) {
            m_upPressed = false;
        }
        if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS && !m_downPressed) {
            Menu::processKey(GLFW_KEY_DOWN);
            m_downPressed = true;
        }
        if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_RELEASE) {
            m_downPressed = false;
        }
        if (glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS && !m_enterPressed) {
            Menu::processKey(GLFW_KEY_ENTER);
            m_enterPressed = true;
        }
        if (glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
            m_enterPressed = false;
        }
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !m_escPressed) {
            Menu::processKey(GLFW_KEY_ESCAPE);
            m_escPressed = true;
        }
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
            m_escPressed = false;
        }
    } else {
        // ESC — exit (only if menu is closed)
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !m_escPressed) {
            glfwSetWindowShouldClose(m_window, true);
            m_escPressed = true;
        }
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
            m_escPressed = false;
        }
    }

    // Camera movement
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera->processKeyboard(FORWARD, dt);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera->processKeyboard(BACKWARD, dt);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera->processKeyboard(LEFT, dt);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera->processKeyboard(RIGHT, dt);
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
        glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
        m_camera->processKeyboard(UP, dt);
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
        glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        m_camera->processKeyboard(DOWN, dt);
}

void Application::update(float dt) {
    if (m_reloadMessageTime > 0.0f) {
        m_reloadMessageTime -= dt;
    }
    
    if (Menu::needsMovementUpdate()) {
        Menu::MovementState state = Menu::getMovementState();
        if (state == Menu::MOVEMENT_RESET) {
            m_rotationX = 0.0f;
            m_rotationY = 0.0f;
            m_rotationZ = 0.0f;
            m_isSpinning = false;
        } else if (state == Menu::MOVEMENT_SPINNING) {
            m_isSpinning = true;
        } else if (state == Menu::MOVEMENT_STOPPED) {
            m_isSpinning = false;
        }
        Menu::markMovementUpdated();
    }
    
    if (m_isSpinning) {
        m_rotationX += dt * 50.0f;
        m_rotationY += dt * 50.0f;
        m_rotationZ += dt * 50.0f;
    }
    
    if (Menu::needsReload()) {
        std::string selectedPath = Menu::getSelectedTexturePath();
        if (selectedPath == "GENERATED_GRID") {
            m_texture->loadGeneratedGrid();
        } else {
            m_texture->loadFromFile(selectedPath);
        }
        Menu::markReloaded();
    }
}

void Application::render() {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    if (width <= 0 || height <= 0) {
        return;
    }
    
    m_renderer->beginFrame(0.10f, 0.12f, 0.16f, 1.0f);
    
    if (m_shader != nullptr && m_shader->m_id != 0 && m_camera != nullptr && m_texture != nullptr) {
        // MVP matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(m_rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(m_rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
        
        glm::mat4 view = m_camera->getViewMatrix();
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glm::mat4 projection = glm::perspective(glm::radians(m_camera->fov), aspect, 0.1f, 100.0f);
        glm::mat4 mvp = projection * view * model;

        m_renderer->drawTexturedCube(*m_shader, m_VAO, *m_texture, mvp);
    }
    
    Menu::update();
    Menu::render();

    // Render UI
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "FPS: " << m_currentFPS << "\n";
    oss << "FOV: " << (m_camera ? m_camera->fov : 45.0f);
    if (m_showGPUInfo) {
        std::string gpuStr = std::string(m_gpuRenderer);
        if (gpuStr.length() > 40) {
            gpuStr = gpuStr.substr(0, 37) + "...";
        }
        oss << "\nGPU: " << gpuStr;
    }
    
    UIText::renderText(oss.str(), 10.0f, 10.0f, 1.5f);
    
    // Show shader reload status
    if (m_reloadMessageTime > 0.0f) {
        UIText::renderTextWithColor(m_reloadMessage, 10.0f, 200.0f, 1.5f, 0.0f, 1.0f, 0.0f);
    }
    
    // Show shader source indicator (if loaded from files)
    if (m_shader != nullptr && m_shader->m_id != 0) {
        UIText::renderTextWithColor("Shaders: Files", 10.0f, 250.0f, 1.2f, 0.0f, 1.0f, 0.0f);
    }
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onFramebufferResize(width, height);
}

void Application::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onMouseMove(xpos, ypos);
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)xoffset;
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onScroll(xoffset, yoffset);
}

void Application::glfwErrorCallback(int error, const char* description) {
    std::cerr << "[GLFW] Error " << error << ": " << description << "\n";
}

void Application::onFramebufferResize(int width, int height) {
    glViewport(0, 0, width, height);
    UIText::updateWindowSize(width, height);
    m_width = width;
    m_height = height;
}

void Application::onMouseMove(double xpos, double ypos) {
    if (m_camera == nullptr) return;
    
    if (m_firstMouse) {
        m_lastX = static_cast<float>(xpos);
        m_lastY = static_cast<float>(ypos);
        m_firstMouse = false;
    }
    
    float xoffset = static_cast<float>(xpos) - m_lastX;
    float yoffset = m_lastY - static_cast<float>(ypos);
    
    m_lastX = static_cast<float>(xpos);
    m_lastY = static_cast<float>(ypos);
    
    m_camera->processMouse(xoffset, yoffset);
}

void Application::onScroll(double xoffset, double yoffset) {
    (void)xoffset;
    if (m_camera == nullptr) return;
    m_camera->processScroll(static_cast<float>(yoffset));
}
