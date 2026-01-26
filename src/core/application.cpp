#include "application.h"
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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <algorithm>

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
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main() {
    FragPos = vec3(u_Model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(u_Model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = u_Projection * u_View * vec4(FragPos, 1.0);
}
)";

    const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 u_CameraPos;
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform int u_UseAlbedoTex;
uniform sampler2D u_AlbedoTex;
uniform vec3 u_AlbedoColor;
uniform vec3 u_SpecularColor;
uniform float u_Shininess;

void main() {
    vec3 albedo = u_UseAlbedoTex != 0 ? texture(u_AlbedoTex, TexCoord).rgb : u_AlbedoColor;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_LightPos - FragPos);
    float ambient = 0.1;
    vec3 ambientColor = ambient * albedo;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseColor = diff * albedo * u_LightColor;
    vec3 viewDir = normalize(u_CameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
    vec3 specularColor = spec * u_SpecularColor * u_LightColor;
    vec3 result = ambientColor + diffuseColor + specularColor;
    FragColor = vec4(result, 1.0);
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

    // Create cube mesh
    m_cubeMesh = MeshFactory::CreateCube();

    // Create materials
    Texture2D* texJager = new Texture2D();
    texJager->loadFromFile("assets/textures/jager.png");
    m_textures.push_back(texJager);
    Material* matJager = new Material(m_shader, texJager);
    matJager->shininess = 32.0f;
    m_materials.push_back(matJager);
    
    Texture2D* texZelya = new Texture2D();
    texZelya->loadFromFile("assets/textures/zelya.png");
    m_textures.push_back(texZelya);
    Material* matZelya = new Material(m_shader, texZelya);
    matZelya->shininess = 8.0f;
    m_materials.push_back(matZelya);
    
    Texture2D* texGrid = new Texture2D();
    texGrid->loadGeneratedGrid();
    m_textures.push_back(texGrid);
    Material* matGrid = new Material(m_shader, texGrid);
    matGrid->shininess = 128.0f;
    m_materials.push_back(matGrid);
    
    // Create objects (5 cubes in a row)
    m_objects.resize(5);
    m_objects[0] = { glm::vec3(-4.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), m_materials[0], false };
    m_objects[1] = { glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), m_materials[1], false };
    m_objects[2] = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), m_materials[2], false };
    m_objects[3] = { glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), m_materials[0], false };
    m_objects[4] = { glm::vec3(4.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), m_materials[1], false };
    
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

    m_gpuRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    
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
}

void Application::shutdown() {
    m_cubeMesh = nullptr;
    if (m_shader) {
        delete m_shader;
    }
    // Cleanup materials
    for (Material* mat : m_materials) {
        delete mat;
    }
    m_materials.clear();
    // Cleanup textures
    for (Texture2D* tex : m_textures) {
        delete tex;
    }
    m_textures.clear();
    if (m_camera) {
        delete m_camera;
    }
    if (m_renderer) {
        delete m_renderer;
    }
    if (m_lightShader) {
        delete m_lightShader;
    }
    if (m_lightVAO != 0) {
        glDeleteVertexArrays(1, &m_lightVAO);
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
        m_jPressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_J) == GLFW_RELEASE) {
        m_jPressed = false;
    }
    
    // K — increase shininess
    if (glfwGetKey(m_window, GLFW_KEY_K) == GLFW_PRESS && !m_kPressed) {
        m_shininess = glm::min(256.0f, m_shininess + 8.0f);
        m_kPressed = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_K) == GLFW_RELEASE) {
        m_kPressed = false;
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
            
            Menu::setCubePosition(obj.position.x, obj.position.y, obj.position.z);
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
            
            Material* newMaterial = nullptr;
            
            if (selectedPath == "GENERATED_GRID") {
                // Grid material is at index 2
                newMaterial = m_materials[2];
            } else {
                // Match texture by filename
                std::string texPath = selectedPath;
                std::replace(texPath.begin(), texPath.end(), '\\', '/');
                if (texPath.find("jager") != std::string::npos) {
                    newMaterial = m_materials[0];
                } else if (texPath.find("zelya") != std::string::npos) {
                    newMaterial = m_materials[1];
                }
            }
            
            if (newMaterial) {
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
        glm::mat4 view = m_camera->getViewMatrix();
        float aspect = static_cast<float>(width) / static_cast<float>(height);
        glm::mat4 projection = glm::perspective(glm::radians(m_camera->fov), aspect, 0.1f, 100.0f);
        
        // Render all objects
        if (m_cubeMesh) {
            for (const RenderObject& obj : m_objects) {
                if (!obj.material) continue;
                
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, obj.position);
                model = glm::rotate(model, glm::radians(obj.rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(obj.rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, glm::radians(obj.rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, obj.scale);
                
                m_renderer->drawMesh(*m_cubeMesh, *obj.material, model, view, projection, m_camera->position, m_dirLight, m_pointLight);
            }
        }
        
        // Render point light sphere
        if (m_pointLight.enabled && m_lightShader != nullptr && m_lightShader->m_id != 0) {
            glm::mat4 lightModel = glm::mat4(1.0f);
            lightModel = glm::translate(lightModel, m_pointLight.position);
            lightModel = glm::scale(lightModel, glm::vec3(0.1f)); // Small sphere
            
            m_lightShader->use();
            m_lightShader->setMat4("u_Model", lightModel);
            m_lightShader->setMat4("u_View", view);
            m_lightShader->setMat4("u_Projection", projection);
            m_lightShader->setVec3("u_LightColor", m_pointLight.color);
            
            glBindVertexArray(m_lightVAO);
            glDrawElements(GL_TRIANGLES, m_lightIndexCount, GL_UNSIGNED_INT, 0);
        }
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
    
    // Lighting UI
    std::ostringstream lightOss;
    lightOss << std::fixed << std::setprecision(0);
    lightOss << "Shininess: " << m_shininess << "\n";
    lightOss << "DirLight: " << (m_dirLight.enabled ? "ON" : "OFF") << "\n";
    lightOss << "PointLight: " << (m_pointLight.enabled ? "ON" : "OFF");
    UIText::renderText(lightOss.str(), 10.0f, 290.0f, 1.2f);
    
    // Selected Cube UI
    std::ostringstream cubeOss;
    cubeOss << "Selected Cube: " << m_selectedObject;
    UIText::renderText(cubeOss.str(), 10.0f, 350.0f, 1.2f);
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
