#include "application.h"
#include "asset_paths.h"
#include "../camera.h"
#include "../shader.h"
#include "../texture2d.h"
#include "../ui_text.h"
#include "../menu.h"
#include "../gl_debug.h"
#include "../renderer/renderer.h"
#include "../renderer/material.h"
#include "../renderer/mesh/mesh_factory.h"
#include "../renderer/light.h"
#include "resource_manager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <algorithm>
#include <utility>

namespace {

constexpr int kBenchmarkColumns = 32;
constexpr int kBenchmarkRows = 18;
constexpr int kBenchmarkLayers = 4;
constexpr int kBenchmarkShaderIterations = 48;
constexpr float kBenchmarkFov = 45.0f;

const char* shaderViewModeName(int mode) {
    switch (mode) {
        case 1: return "Albedo";
        case 2: return "Normals";
        case 3: return "Diffuse";
        case 4: return "Specular";
        default: return "Lit";
    }
}

} // namespace

Application::Application() {
    m_initialized = init();
}

Application::~Application() {
    shutdown();
}

bool Application::init() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    m_glfwInitialized = true;

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        std::cerr << "Failed to get primary monitor\n";
        return false;
    }

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        std::cerr << "Failed to get primary monitor video mode\n";
        return false;
    }

    m_window = glfwCreateWindow(mode->width, mode->height, "Graphics_engine", monitor, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window\n";
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);

    // VSync 1 - on 0 - off
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    m_contextReady = true;

    const auto* rendererName = glGetString(GL_RENDERER);
    m_gpuRenderer = rendererName != nullptr
        ? reinterpret_cast<const char*>(rendererName)
        : "Unknown GPU";

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << m_gpuRenderer << "\n";
    std::cout << "Assets: " << core::findAssetsRoot().string() << "\n";

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

    // Shaders
    m_shader = ResourceManager::getShader("shaders/textured.vert", "shaders/textured.frag");
    if (!m_shader || m_shader->m_id == 0) {
        std::cerr << "Failed to create shader program\n";
        return false;
    }

    // Create cube mesh
    m_cubeMesh = MeshFactory::CreateCube();

    // Create the initial materials through the same cache used by menu selections.
    Material* matJager = getOrCreateMaterial("textures/jager.png");
    Material* matZelya = getOrCreateMaterial("textures/zelya.png");
    Material* matGrid = getOrCreateMaterial("textures/generated_grid");
    if (!matJager || !matZelya || !matGrid) {
        std::cerr << "Failed to create initial materials\n";
        return false;
    }
    
    // Create objects (5 cubes in a row)
    m_objects.resize(5);
    m_objects[0] = { glm::vec3(-4.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), matJager, false };
    m_objects[1] = { glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), matZelya, false };
    m_objects[2] = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), matGrid, false };
    m_objects[3] = { glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), matJager, false };
    m_objects[4] = { glm::vec3(4.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), matZelya, false };

    m_benchmarkMaterial = std::make_unique<Material>(m_shader.get(), matGrid->albedo);
    m_benchmarkMaterial->shininess = 32.0f;
    if (!initBenchmarkScene()) {
        std::cerr << "Failed to create benchmark scene\n";
        return false;
    }
    
    m_selectedObject = 0;
    
    // Generate sphere for light visualization
    const int segments = 32;
    const int rings = 32;
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    
    // Generate vertices
    for (int i = 0; i <= rings; ++i) {
        float theta = glm::pi<float>() * i / rings;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        
        for (int j = 0; j <= segments; ++j) {
            float phi = 2.0f * glm::pi<float>() * j / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
        }
    }
    
    // Generate indices
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = i * (segments + 1) + j;
            int second = first + segments + 1;
            
            sphereIndices.push_back(first);
            sphereIndices.push_back(second);
            sphereIndices.push_back(first + 1);
            
            sphereIndices.push_back(second);
            sphereIndices.push_back(second + 1);
            sphereIndices.push_back(first + 1);
        }
    }
    
    m_lightIndexCount = static_cast<int>(sphereIndices.size());
    
    // Light sphere VAO
    glGenVertexArrays(1, &m_lightVAO);
    glGenBuffers(1, &m_lightVBO);
    glGenBuffers(1, &m_lightEBO);
    
    glBindVertexArray(m_lightVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lightEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Simple shader for light cube (no lighting, just color)
    const char* lightVertexSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);
}
)";

    const char* lightFragmentSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 u_LightColor;

void main() {
    FragColor = vec4(u_LightColor, 1.0);
}
)";

    m_lightShader = new Shader(lightVertexSource, lightFragmentSource);

    // Renderer
    m_renderer = new Renderer();
    m_renderer->init();
    
    // Texture menu
    Menu::init();
    
    // Sync initial light position with menu
    Menu::setLightPosition(m_pointLight.position.x, m_pointLight.position.y, m_pointLight.position.z);
    if (!m_objects.empty()) {
        Menu::setSelectedCubeIndex(m_selectedObject);
        Menu::setCubePosition(m_objects[m_selectedObject].position.x, 
                             m_objects[m_selectedObject].position.y, 
                             m_objects[m_selectedObject].position.z);
    }

    return true;
}

bool Application::initBenchmarkScene() {
    if (!m_cubeMesh || m_benchmarkMaterial == nullptr) {
        return false;
    }

    std::vector<glm::mat4> transforms;
    transforms.reserve(kBenchmarkColumns * kBenchmarkRows * kBenchmarkLayers);

    for (int layer = 0; layer < kBenchmarkLayers; ++layer) {
        for (int row = 0; row < kBenchmarkRows; ++row) {
            for (int column = 0; column < kBenchmarkColumns; ++column) {
                const float x = (static_cast<float>(column) - (kBenchmarkColumns - 1) * 0.5f) * 1.15f
                              + (layer % 2 == 0 ? 0.0f : 0.35f);
                const float y = (static_cast<float>(row) - (kBenchmarkRows - 1) * 0.5f) * 1.15f;
                const float z = -static_cast<float>(layer) * 1.4f;

                glm::mat4 model(1.0f);
                model = glm::translate(model, glm::vec3(x, y, z));
                const float angle = glm::radians(static_cast<float>((column * 13 + row * 7 + layer * 19) % 360));
                const glm::vec3 axis = glm::normalize(glm::vec3(
                    0.35f + static_cast<float>(row % 3) * 0.1f,
                    1.0f,
                    0.25f + static_cast<float>(column % 5) * 0.05f));
                model = glm::rotate(model, angle, axis);
                model = glm::scale(model, glm::vec3(0.92f));
                transforms.push_back(model);
            }
        }
    }

    m_benchmarkInstanceCount = static_cast<GLsizei>(transforms.size());
    auto instanceVbo = std::make_unique<VertexBuffer>(
        transforms.data(), transforms.size() * sizeof(glm::mat4), GL_STATIC_DRAW);
    if (instanceVbo->getId() == 0) {
        m_benchmarkInstanceCount = 0;
        return false;
    }

    BufferLayout instanceLayout({
        { ShaderDataType::Float4, "aInstanceModel0" },
        { ShaderDataType::Float4, "aInstanceModel1" },
        { ShaderDataType::Float4, "aInstanceModel2" },
        { ShaderDataType::Float4, "aInstanceModel3" }
    });
    m_cubeMesh->addVertexBuffer(std::move(instanceVbo), instanceLayout, 3, 1);
    return true;
}

Material* Application::getOrCreateMaterial(const std::string& relativeTexturePath) {
    if (!m_shader || relativeTexturePath.empty()) {
        return nullptr;
    }

    std::string cacheKey = relativeTexturePath;
    std::replace(cacheKey.begin(), cacheKey.end(), '\\', '/');

    auto existing = m_materials.find(cacheKey);
    if (existing != m_materials.end()) {
        return existing->second.get();
    }

    std::shared_ptr<Texture2D> texture = ResourceManager::getTexture(cacheKey);
    if (!texture) {
        return nullptr;
    }

    auto material = std::make_unique<Material>(m_shader.get(), texture.get());
    material->shininess = m_shininess;
    Material* materialPtr = material.get();

    m_textures.emplace(cacheKey, std::move(texture));
    m_materials.emplace(cacheKey, std::move(material));
    return materialPtr;
}

void Application::applyGlobalShininess() {
    for (auto& [path, material] : m_materials) {
        (void)path;
        if (material) {
            material->shininess = m_shininess;
        }
    }
}

void Application::resetFrameStatistics() {
    m_fpsUpdateTime = 0.0f;
    m_frameCount = 0;
    m_currentFPS = 0.0f;
    m_cpuFrameTimeMs = 0.0f;
    m_presentTimeMs = 0.0f;
    m_hasFrameStatistics = false;
    m_hasPresentTime = false;
    m_skipNextFrameSample = true;
    m_lastFrame = static_cast<float>(glfwGetTime());

    if (m_renderer != nullptr) {
        m_renderer->resetGpuFrameTimes();
    }
}

void Application::toggleBenchmark() {
    m_benchmarkEnabled = !m_benchmarkEnabled;
    if (m_benchmarkEnabled && Menu::isOpen()) {
        Menu::toggle();
    }
    m_firstMouse = true;
    resetFrameStatistics();

    std::cout << "GPU benchmark " << (m_benchmarkEnabled ? "enabled" : "disabled") << "\n";
}

void Application::shutdown() {
    m_initialized = false;

    if (m_contextReady && m_window) {
        glfwMakeContextCurrent(m_window);
    }

    m_benchmarkInstanceCount = 0;
    m_benchmarkMaterial.reset();

    m_objects.clear();
    m_materials.clear();
    m_cubeMesh.reset();
    m_textures.clear();
    m_shader.reset();

    if (m_camera) {
        delete m_camera;
        m_camera = nullptr;
    }
    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }

    if (m_contextReady) {
        if (m_lightShader) {
            delete m_lightShader;
            m_lightShader = nullptr;
        }
        if (m_lightEBO != 0) {
            glDeleteBuffers(1, &m_lightEBO);
            m_lightEBO = 0;
        }
        if (m_lightVBO != 0) {
            glDeleteBuffers(1, &m_lightVBO);
            m_lightVBO = 0;
        }
        if (m_lightVAO != 0) {
            glDeleteVertexArrays(1, &m_lightVAO);
            m_lightVAO = 0;
        }

        UIText::cleanup();
        ResourceManager::clear();
    }

    m_lightShader = nullptr;
    m_lightIndexCount = 0;
    m_gpuRenderer.clear();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    m_contextReady = false;

    if (m_glfwInitialized) {
        glfwTerminate();
        m_glfwInitialized = false;
    }
}

int Application::run() {
    if (!m_initialized || !m_window || !m_renderer) {
        return 1;
    }

    m_lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(m_window)) {
        // Delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        m_deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        processInput(m_deltaTime);
        update(m_deltaTime);
        render();
        
        m_renderer->endFrame();

        const double presentStart = glfwGetTime();
        glfwSwapBuffers(m_window);
        const float presentSampleMs = static_cast<float>((glfwGetTime() - presentStart) * 1000.0);
        if (m_hasPresentTime) {
            constexpr float smoothingFactor = 0.1f;
            m_presentTimeMs += (presentSampleMs - m_presentTimeMs) * smoothingFactor;
        } else {
            m_presentTimeMs = presentSampleMs;
            m_hasPresentTime = true;
        }

        glfwPollEvents();
    }

    return 0;
}

void Application::processInput(float dt) {
    // FPS
    if (m_skipNextFrameSample) {
        m_skipNextFrameSample = false;
    } else {
        m_frameCount++;
        m_fpsUpdateTime += dt;
        if (m_fpsUpdateTime >= 0.5f) {
            const float sampledFrameCount = static_cast<float>(m_frameCount);
            m_currentFPS = sampledFrameCount / m_fpsUpdateTime;
            m_cpuFrameTimeMs = (m_fpsUpdateTime * 1000.0f) / sampledFrameCount;
            m_frameCount = 0;
            m_fpsUpdateTime = 0.0f;
            m_hasFrameStatistics = true;
        }
    }

    if (m_benchmarkEnabled) {
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !m_escPressed) {
            glfwSetWindowShouldClose(m_window, true);
            m_escPressed = true;
        }
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_RELEASE) {
            m_escPressed = false;
        }
        return;
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
        m_reloadSucceeded = ResourceManager::reloadAllShaders();
        m_reloadMessage = m_reloadSucceeded ? "Shaders reloaded" : "Shader reload failed";
        m_reloadMessageTime = 2.0f;
        m_f5Pressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_F5) == GLFW_RELEASE) {
        m_f5Pressed = false;
    }

    // Shader inspection views. Reassigning while held is harmless and avoids debounce state.
    if (glfwGetKey(m_window, GLFW_KEY_F1) == GLFW_PRESS) m_shaderViewMode = 0;
    if (glfwGetKey(m_window, GLFW_KEY_F2) == GLFW_PRESS) m_shaderViewMode = 1;
    if (glfwGetKey(m_window, GLFW_KEY_F3) == GLFW_PRESS) m_shaderViewMode = 2;
    if (glfwGetKey(m_window, GLFW_KEY_F4) == GLFW_PRESS) m_shaderViewMode = 3;
    if (glfwGetKey(m_window, GLFW_KEY_F6) == GLFW_PRESS) m_shaderViewMode = 4;
    
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
    
    // L — toggle point light on/off
    if (glfwGetKey(m_window, GLFW_KEY_L) == GLFW_PRESS && !m_lPressed) {
        m_pointLight.enabled = !m_pointLight.enabled;
        m_lPressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_L) == GLFW_RELEASE) {
        m_lPressed = false;
    }
    
    // O — toggle directional light on/off
    if (glfwGetKey(m_window, GLFW_KEY_O) == GLFW_PRESS && !m_oPressed) {
        m_dirLight.enabled = !m_dirLight.enabled;
        m_oPressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_O) == GLFW_RELEASE) {
        m_oPressed = false;
    }
    
    // J — decrease shininess
    if (glfwGetKey(m_window, GLFW_KEY_J) == GLFW_PRESS && !m_jPressed) {
        m_shininess = glm::max(2.0f, m_shininess - 8.0f);
        applyGlobalShininess();
        m_jPressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_J) == GLFW_RELEASE) {
        m_jPressed = false;
    }
    
    // K — increase shininess
    if (glfwGetKey(m_window, GLFW_KEY_K) == GLFW_PRESS && !m_kPressed) {
        m_shininess = glm::min(256.0f, m_shininess + 8.0f);
        applyGlobalShininess();
        m_kPressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_K) == GLFW_RELEASE) {
        m_kPressed = false;
    }

    // Camera movement is frozen while the benchmark uses its fixed view.
    if (!m_benchmarkEnabled) {
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
}

void Application::update(float dt) {
    if (m_reloadMessageTime > 0.0f) {
        m_reloadMessageTime -= dt;
    }

    if (m_benchmarkEnabled) {
        return;
    }
    
    if (Menu::needsCubeUpdate()) {
        Menu::CubeControlAction action = Menu::getCubeControlAction();
        const float step = 0.5f;
        
        // Sync selected object index with menu
        m_selectedObject = Menu::getSelectedCubeIndex();
        if (m_selectedObject < 0) m_selectedObject = 0;
        if (m_selectedObject >= static_cast<int>(m_objects.size())) {
            m_selectedObject = static_cast<int>(m_objects.size()) - 1;
        }
        
        if (m_selectedObject >= 0 && m_selectedObject < static_cast<int>(m_objects.size())) {
            RenderObject& obj = m_objects[m_selectedObject];
            
            switch (action) {
                case Menu::CUBE_PREV:
                    m_selectedObject--;
                    if (m_selectedObject < 0) m_selectedObject = static_cast<int>(m_objects.size()) - 1;
                    Menu::setSelectedCubeIndex(m_selectedObject);
                    break;
                case Menu::CUBE_NEXT:
                    m_selectedObject++;
                    if (m_selectedObject >= static_cast<int>(m_objects.size())) m_selectedObject = 0;
                    Menu::setSelectedCubeIndex(m_selectedObject);
                    break;
                case Menu::CUBE_RESET:
                    obj.rotationDeg = glm::vec3(0.0f);
                    obj.spinning = false;
                    obj.position = glm::vec3(0.0f);
                    obj.scale = glm::vec3(1.0f);
                    break;
                case Menu::CUBE_SPIN:
                    obj.spinning = true;
                    break;
                case Menu::CUBE_STOP:
                    obj.spinning = false;
                    break;
                case Menu::CUBE_X_INC: obj.position.x += step; break;
                case Menu::CUBE_X_DEC: obj.position.x -= step; break;
                case Menu::CUBE_Y_INC: obj.position.y += step; break;
                case Menu::CUBE_Y_DEC: obj.position.y -= step; break;
                case Menu::CUBE_Z_INC: obj.position.z += step; break;
                case Menu::CUBE_Z_DEC: obj.position.z -= step; break;
                case Menu::CUBE_NONE: break;
            }
            
            const RenderObject& selectedObj = m_objects[m_selectedObject];
            Menu::setCubePosition(selectedObj.position.x, selectedObj.position.y, selectedObj.position.z);
        }
        Menu::markCubeUpdated();
    } else {
        // Sync selected object index with menu
        m_selectedObject = Menu::getSelectedCubeIndex();
        if (m_selectedObject < 0) m_selectedObject = 0;
        if (m_selectedObject >= static_cast<int>(m_objects.size())) {
            m_selectedObject = static_cast<int>(m_objects.size()) - 1;
        }
        Menu::setSelectedCubeIndex(m_selectedObject);
        
        if (m_selectedObject >= 0 && m_selectedObject < static_cast<int>(m_objects.size())) {
            const RenderObject& obj = m_objects[m_selectedObject];
            Menu::setCubePosition(obj.position.x, obj.position.y, obj.position.z);
        }
    }
    
    // Update spinning for all objects
    for (RenderObject& obj : m_objects) {
        if (obj.spinning) {
            obj.rotationDeg.x += dt * 50.0f;
            obj.rotationDeg.y += dt * 50.0f;
            obj.rotationDeg.z += dt * 50.0f;
        }
    }

    if (m_pointLightSpinning) {
        m_pointLightSpinAngle += dt;
        const float radius = 3.0f;
        m_pointLight.position.x = std::cos(m_pointLightSpinAngle) * radius;
        m_pointLight.position.z = std::sin(m_pointLightSpinAngle) * radius;
    }
    
    if (Menu::needsReload()) {
        std::string selectedPath = Menu::getSelectedTexturePath();
        
        // Update material for selected cube
        if (m_selectedObject >= 0 && m_selectedObject < static_cast<int>(m_objects.size())) {
            RenderObject& obj = m_objects[m_selectedObject];
            if (Material* newMaterial = getOrCreateMaterial(selectedPath)) {
                obj.material = newMaterial;
            }
        }
        
        Menu::markReloaded();
    }
    
    if (Menu::needsLightUpdate()) {
        Menu::LightControlAction action = Menu::getLightControlAction();
        const float step = 0.5f;
        
        switch (action) {
            case Menu::LIGHT_X_INC:
                m_pointLight.position.x += step;
                break;
            case Menu::LIGHT_X_DEC:
                m_pointLight.position.x -= step;
                break;
            case Menu::LIGHT_Y_INC:
                m_pointLight.position.y += step;
                break;
            case Menu::LIGHT_Y_DEC:
                m_pointLight.position.y -= step;
                break;
            case Menu::LIGHT_Z_INC:
                m_pointLight.position.z += step;
                break;
            case Menu::LIGHT_Z_DEC:
                m_pointLight.position.z -= step;
                break;
            case Menu::LIGHT_RESET:
                m_pointLightSpinning = false;
                m_pointLightSpinAngle = 0.0f;
                m_pointLight.position = glm::vec3(2.0f, 2.0f, 2.0f);
                break;
            case Menu::LIGHT_SPIN:
                m_pointLightSpinning = true;
                break;
            case Menu::LIGHT_STOP:
                m_pointLightSpinning = false;
                break;
            // XY Plane movements
            case Menu::LIGHT_XY_UP:
                m_pointLight.position.y += step;
                break;
            case Menu::LIGHT_XY_DOWN:
                m_pointLight.position.y -= step;
                break;
            case Menu::LIGHT_XY_LEFT:
                m_pointLight.position.x -= step;
                break;
            case Menu::LIGHT_XY_RIGHT:
                m_pointLight.position.x += step;
                break;
            // XZ Plane movements
            case Menu::LIGHT_XZ_FORWARD:
                m_pointLight.position.z += step;
                break;
            case Menu::LIGHT_XZ_BACK:
                m_pointLight.position.z -= step;
                break;
            case Menu::LIGHT_XZ_LEFT:
                m_pointLight.position.x -= step;
                break;
            case Menu::LIGHT_XZ_RIGHT:
                m_pointLight.position.x += step;
                break;
            // YZ Plane movements
            case Menu::LIGHT_YZ_UP:
                m_pointLight.position.y += step;
                break;
            case Menu::LIGHT_YZ_DOWN:
                m_pointLight.position.y -= step;
                break;
            case Menu::LIGHT_YZ_FORWARD:
                m_pointLight.position.z += step;
                break;
            case Menu::LIGHT_YZ_BACK:
                m_pointLight.position.z -= step;
                break;
            case Menu::LIGHT_NONE:
                break;
        }
        
        Menu::setLightPosition(m_pointLight.position.x, m_pointLight.position.y, m_pointLight.position.z);
        Menu::markLightUpdated();
    } else {
        // Update menu display with current light position
        Menu::setLightPosition(m_pointLight.position.x, m_pointLight.position.y, m_pointLight.position.z);
    }
    
    // Handle directional light updates from menu
    if (Menu::needsDirLightUpdate()) {
        Menu::DirLightControlAction action = Menu::getDirLightControlAction();
        const float angleStep = 0.1f;
        
        switch (action) {
            case Menu::DIRLIGHT_TOGGLE:
                m_dirLight.enabled = !m_dirLight.enabled;
                break;
            case Menu::DIRLIGHT_ROTATE_LEFT:
                // Rotate around Y axis (yaw)
                {
                    float yaw = std::atan2(m_dirLight.direction.x, m_dirLight.direction.z);
                    yaw -= angleStep;
                    float pitch = std::asin(-m_dirLight.direction.y);
                    m_dirLight.direction.x = std::sin(yaw) * std::cos(pitch);
                    m_dirLight.direction.z = std::cos(yaw) * std::cos(pitch);
                    m_dirLight.direction = glm::normalize(m_dirLight.direction);
                }
                break;
            case Menu::DIRLIGHT_ROTATE_RIGHT:
                // Rotate around Y axis (yaw)
                {
                    float yaw = std::atan2(m_dirLight.direction.x, m_dirLight.direction.z);
                    yaw += angleStep;
                    float pitch = std::asin(-m_dirLight.direction.y);
                    m_dirLight.direction.x = std::sin(yaw) * std::cos(pitch);
                    m_dirLight.direction.z = std::cos(yaw) * std::cos(pitch);
                    m_dirLight.direction = glm::normalize(m_dirLight.direction);
                }
                break;
            case Menu::DIRLIGHT_TILT_UP:
                // Change pitch
                {
                    float yaw = std::atan2(m_dirLight.direction.x, m_dirLight.direction.z);
                    float pitch = std::asin(-m_dirLight.direction.y);
                    pitch = glm::clamp(pitch - angleStep, -glm::pi<float>() / 2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);
                    m_dirLight.direction.x = std::sin(yaw) * std::cos(pitch);
                    m_dirLight.direction.y = -std::sin(pitch);
                    m_dirLight.direction.z = std::cos(yaw) * std::cos(pitch);
                    m_dirLight.direction = glm::normalize(m_dirLight.direction);
                }
                break;
            case Menu::DIRLIGHT_TILT_DOWN:
                // Change pitch
                {
                    float yaw = std::atan2(m_dirLight.direction.x, m_dirLight.direction.z);
                    float pitch = std::asin(-m_dirLight.direction.y);
                    pitch = glm::clamp(pitch + angleStep, -glm::pi<float>() / 2.0f + 0.1f, glm::pi<float>() / 2.0f - 0.1f);
                    m_dirLight.direction.x = std::sin(yaw) * std::cos(pitch);
                    m_dirLight.direction.y = -std::sin(pitch);
                    m_dirLight.direction.z = std::cos(yaw) * std::cos(pitch);
                    m_dirLight.direction = glm::normalize(m_dirLight.direction);
                }
                break;
            case Menu::POINTLIGHT_TOGGLE:
                m_pointLight.enabled = !m_pointLight.enabled;
                break;
            case Menu::DIRLIGHT_NONE:
                break;
        }
        
        Menu::markDirLightUpdated();
    }
}

void Application::render() {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    if (width <= 0 || height <= 0) {
        return;
    }
    
    m_renderer->beginFrame(0.10f, 0.12f, 0.16f, 1.0f);
    
    if (m_shader != nullptr && m_shader->m_id != 0 && m_camera != nullptr) {
        const float aspect = static_cast<float>(width) / static_cast<float>(height);

        if (m_benchmarkEnabled && m_cubeMesh && m_benchmarkMaterial != nullptr) {
            const glm::vec3 cameraPos(0.0f, 0.0f, 28.0f);
            const glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat4 projection = glm::perspective(glm::radians(kBenchmarkFov), aspect, 0.1f, 100.0f);
            DirectionalLight benchmarkDirLight;
            PointLight benchmarkPointLight;
            benchmarkPointLight.enabled = false;

            m_renderer->drawMeshInstanced(*m_cubeMesh, *m_benchmarkMaterial,
                                          m_benchmarkInstanceCount, view, projection,
                                          cameraPos, benchmarkDirLight, benchmarkPointLight,
                                          kBenchmarkShaderIterations);
        } else {
            const glm::mat4 view = m_camera->getViewMatrix();
            const glm::mat4 projection = glm::perspective(glm::radians(m_camera->fov), aspect, 0.1f, 100.0f);

            if (m_cubeMesh) {
                for (const RenderObject& obj : m_objects) {
                    if (!obj.material) continue;

                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, obj.position);
                    model = glm::rotate(model, glm::radians(obj.rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, glm::radians(obj.rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::rotate(model, glm::radians(obj.rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
                    model = glm::scale(model, obj.scale);

                    m_renderer->drawMesh(*m_cubeMesh, *obj.material, model, view, projection,
                                         m_camera->position, m_dirLight, m_pointLight, m_shaderViewMode);
                }
            }

            if (m_pointLight.enabled && m_lightShader != nullptr && m_lightShader->m_id != 0) {
                glm::mat4 lightModel = glm::mat4(1.0f);
                lightModel = glm::translate(lightModel, m_pointLight.position);
                lightModel = glm::scale(lightModel, glm::vec3(0.1f));

                m_lightShader->use();
                m_lightShader->setMat4("u_Model", lightModel);
                m_lightShader->setMat4("u_View", view);
                m_lightShader->setMat4("u_Projection", projection);
                m_lightShader->setVec3("u_LightColor", m_pointLight.color);

                glBindVertexArray(m_lightVAO);
                glDrawElements(GL_TRIANGLES, m_lightIndexCount, GL_UNSIGNED_INT, 0);
            }
        }
    }
    
    Menu::update();
    m_renderer->beginUiPass();
    UIText::beginFrame();
    Menu::render();

    // Render UI
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    if (m_hasFrameStatistics) {
        oss << "FPS: " << m_currentFPS << "\n";
        oss << std::setprecision(2);
        oss << "Frame interval: " << m_cpuFrameTimeMs << " ms\n";
    } else {
        oss << "FPS: warming up\n";
        oss << "Frame interval: warming up\n";
    }
    if (m_hasPresentTime) {
        oss << std::setprecision(2);
        oss << "Present: " << m_presentTimeMs << " ms\n";
    } else {
        oss << "Present: warming up\n";
    }
    if (m_renderer->hasGpuFrameTime()) {
        oss << "GPU total: " << m_renderer->gpuFrameTimeMs() << " ms\n";
        oss << "  Scene: " << m_renderer->gpuSceneTimeMs() << " ms\n";
        oss << "  UI: " << m_renderer->gpuUiTimeMs() << " ms\n";
    } else {
        oss << "GPU timings: warming up\n";
    }
    oss << std::setprecision(1);
    oss << "FOV: " << (m_benchmarkEnabled ? kBenchmarkFov : (m_camera ? m_camera->fov : 45.0f));
    if (m_benchmarkEnabled) {
        oss << " (fixed)";
    }
    oss << "\nBenchmark: " << (m_benchmarkEnabled ? "ON" : "OFF") << " [F7]";
    if (m_benchmarkEnabled) {
        oss << "\nResolution: " << width << "x" << height;
        oss << "\nInstances: " << m_benchmarkInstanceCount;
        oss << "\nShader loops: " << kBenchmarkShaderIterations;
        oss << "\nCompare GPUs by Scene time";
    }
    if (m_showGPUInfo || m_benchmarkEnabled) {
        std::string gpuStr = m_gpuRenderer;
        if (gpuStr.length() > 40) {
            gpuStr = gpuStr.substr(0, 37) + "...";
        }
        oss << "\nGPU: " << gpuStr;
    }
    
    UIText::renderText(oss.str(), 10.0f, 10.0f, 1.5f);
    
    // Show shader reload status
    if (m_reloadMessageTime > 0.0f) {
        const float red = m_reloadSucceeded ? 0.0f : 1.0f;
        const float green = m_reloadSucceeded ? 1.0f : 0.0f;
        UIText::renderTextWithColor(m_reloadMessage, 10.0f, 200.0f, 1.5f, red, green, 0.0f);
    }
    
    // Show active shader inspection view.
    if (!m_benchmarkEnabled && m_shader != nullptr && m_shader->m_id != 0) {
        std::string shaderStatus = std::string("Shader: ") + shaderViewModeName(m_shaderViewMode) +
                                   " [F1-F4/F6]";
        UIText::renderTextWithColor(shaderStatus, 10.0f, 250.0f, 1.2f, 0.0f, 1.0f, 0.0f);
    }

    if (!m_benchmarkEnabled) {
        std::ostringstream lightOss;
        lightOss << std::fixed << std::setprecision(0);
        lightOss << "Shininess: " << m_shininess << "\n";
        lightOss << "DirLight: " << (m_dirLight.enabled ? "ON" : "OFF") << "\n";
        lightOss << "PointLight: " << (m_pointLight.enabled ? "ON" : "OFF");
        UIText::renderText(lightOss.str(), 10.0f, 290.0f, 1.2f);

        std::ostringstream cubeOss;
        cubeOss << "Selected Cube: " << m_selectedObject;
        UIText::renderText(cubeOss.str(), 10.0f, 350.0f, 1.2f);
    }
    UIText::flush();
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onFramebufferResize(width, height);
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onKey(key, action);
    }
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

void Application::onKey(int key, int action) {
    if (key == GLFW_KEY_F7 && action == GLFW_PRESS) {
        toggleBenchmark();
    }
}

void Application::onMouseMove(double xpos, double ypos) {
    if (m_camera == nullptr) return;

    if (m_benchmarkEnabled) {
        m_lastX = static_cast<float>(xpos);
        m_lastY = static_cast<float>(ypos);
        return;
    }
    
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
    if (m_camera == nullptr || m_benchmarkEnabled) return;
    m_camera->processScroll(static_cast<float>(yoffset));
}
