#include <iostream>
#include <sstream>
#include <iomanip>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_debug.h"
#include "shader.h"
#include "camera.h"
#include "ui_text.h"
#include "texture2d.h"
#include "texture_menu.h"

Camera* g_camera = nullptr;
bool g_firstMouse = true;
float g_lastX = 640.0f;
float g_lastY = 360.0f;

static void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    UIText::updateWindowSize(width, height);
}

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "[GLFW] Error " << error << ": " << description << "\n";
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    if (g_camera == nullptr) return;
    
    if (g_firstMouse) {
        g_lastX = static_cast<float>(xpos);
        g_lastY = static_cast<float>(ypos);
        g_firstMouse = false;
    }
    
    float xoffset = static_cast<float>(xpos) - g_lastX;
    float yoffset = g_lastY - static_cast<float>(ypos);
    
    g_lastX = static_cast<float>(xpos);
    g_lastY = static_cast<float>(ypos);
    
    g_camera->processMouse(xoffset, yoffset);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    (void)window;
    (void)xoffset;
    if (g_camera == nullptr) return;
    g_camera->processScroll(static_cast<float>(yoffset));
}

int main() {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Graphics_engine", monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // VSync 1 - on 0 - off
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // Enable debug output
    GL_EnableDebugOutput();

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    // UI text
    UIText::init(windowWidth, windowHeight);

    // Camera
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    g_camera = &camera;

    // Mouse callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Shaders
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

    Shader shader(vertexShaderSource, fragmentShaderSource);
    if (shader.m_id == 0) {
        std::cerr << "Failed to create shader program\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
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

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    Texture2D texture;
    texture.loadGeneratedGrid();
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float lastFrame = 0.0f;
    float fpsUpdateTime = 0.0f;
    int frameCount = 0;
    float currentFPS = 0.0f;
    
    // GPU info toggle
    bool showGPUInfo = false;
    bool f9Pressed = false;
    bool f8Pressed = false;
    bool upPressed = false;
    bool downPressed = false;
    bool enterPressed = false;
    const char* gpuRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    
    // Texture menu
    TextureMenu::init();

    while (!glfwWindowShouldClose(window)) {
        // Delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // FPS
        frameCount++;
        fpsUpdateTime += deltaTime;
        if (fpsUpdateTime >= 0.5f) {
            currentFPS = frameCount / fpsUpdateTime;
            frameCount = 0;
            fpsUpdateTime = 0.0f;
        }

        // ESC — exit
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // F9 — toggle GPU info
        if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS && !f9Pressed) {
            showGPUInfo = !showGPUInfo;
            f9Pressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_RELEASE) {
            f9Pressed = false;
        }
        
        // F8 — toggle texture menu
        if (glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS && !f8Pressed) {
            TextureMenu::toggle();
            f8Pressed = true;
            if (TextureMenu::isOpen()) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_F8) == GLFW_RELEASE) {
            f8Pressed = false;
        }
        
        if (TextureMenu::isOpen()) {
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && !upPressed) {
                TextureMenu::processKey(265); // GLFW_KEY_UP
                upPressed = true;
            }
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE) {
                upPressed = false;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && !downPressed) {
                TextureMenu::processKey(264); // GLFW_KEY_DOWN
                downPressed = true;
            }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE) {
                downPressed = false;
            }
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !enterPressed) {
                TextureMenu::processKey(257); // GLFW_KEY_ENTER
                enterPressed = true;
                if (!TextureMenu::isOpen()) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
                enterPressed = false;
            }
        }

        // Camera movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            camera.processKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
            camera.processKeyboard(DOWN, deltaTime);

        glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // MVP matrix
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);


        if (width <= 0 || height <= 0) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }
        
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glm::mat4 projection = glm::perspective(glm::radians(camera.fov), aspect, 0.1f, 100.0f);
        
        if (TextureMenu::needsReload()) {
            std::string selectedPath = TextureMenu::getSelectedTexturePath();
            if (selectedPath == "GENERATED_GRID") {
                texture.loadGeneratedGrid();
            } else {
                texture.loadFromFile(selectedPath);
            }
            TextureMenu::markReloaded();
        }
        
        glm::mat4 mvp = projection * view * model;

        shader.use();
        shader.setInt("u_Texture", 0);
        texture.bind(0);
        shader.setMat4("u_MVP", mvp);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        TextureMenu::update();
        TextureMenu::render();

        // Render UI
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        oss << "FPS: " << currentFPS << "\n";
        oss << "FOV: " << camera.fov;
        if (showGPUInfo) {
            std::string gpuStr = std::string(gpuRenderer);
            if (gpuStr.length() > 40) {
                gpuStr = gpuStr.substr(0, 37) + "...";
            }
            oss << "\nGPU: " << gpuStr;
        }
        
        UIText::renderText(oss.str(), 10.0f, 10.0f, 1.5f);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    UIText::cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
