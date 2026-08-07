#include "application.h"
#include "asset_paths.h"
#include "benchmark_report.h"
#include "editor_camera.h"
#include "editor_placement.h"
#include "spatial_query.h"
#include "../camera.h"
#include "../shader.h"
#include "../texture2d.h"
#include "../ui_text.h"
#include "../menu.h"
#include "../gl_debug.h"
#include "../renderer/renderer.h"
#include "../renderer/coordinate_grid.h"
#include "../renderer/selection_outline.h"
#include "../renderer/framebuffer.h"
#include "../renderer/post_processor.h"
#include "../renderer/material.h"
#include "../renderer/model.h"
#include "../renderer/model_loader.h"
#include "../renderer/mesh/mesh_factory.h"
#include "../renderer/mesh/mesh.h"
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
#include <numeric>
#include <limits>
#include <unordered_set>
#include <utility>

namespace {

constexpr int kBenchmarkColumns = 32;
constexpr int kBenchmarkRows = 18;
constexpr int kBenchmarkLayers = 4;
constexpr int kBenchmarkShaderIterations = 48;
constexpr float kBenchmarkFov = 45.0f;
constexpr float kCameraNearPlane = 0.1f;
constexpr float kCameraFarPlane = 100.0f;
constexpr GLsizei kBenchmarkWidth = 1600;
constexpr GLsizei kBenchmarkHeight = 900;
constexpr double kBenchmarkWarmupSeconds = 2.0;
constexpr std::size_t kBenchmarkCaptureSamples = 300;

glm::mat4 objectTransform(const RenderObject& object) {
    glm::mat4 model(1.0f);
    model = glm::translate(model, object.position);
    model = glm::rotate(
        model, glm::radians(object.rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(
        model, glm::radians(object.rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(
        model, glm::radians(object.rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
    return glm::scale(model, object.scale);
}

std::optional<geometry::AxisAlignedBounds> objectWorldBounds(
    const RenderObject& object) {
    const glm::mat4 transform = objectTransform(object);
    std::vector<geometry::AxisAlignedBounds> worldPartBounds;
    if (object.model != nullptr) {
        worldPartBounds.reserve(object.model->parts().size());
        for (const ModelPart& part : object.model->parts()) {
            if (part.mesh != nullptr) {
                if (const auto bounds =
                        core::transformBounds(part.mesh->bounds(), transform);
                    bounds.has_value()) {
                    worldPartBounds.push_back(*bounds);
                }
            }
        }
    } else if (object.mesh != nullptr) {
        if (const auto bounds = core::transformBounds(object.mesh->bounds(), transform);
            bounds.has_value()) {
            worldPartBounds.push_back(*bounds);
        }
    }

    return core::mergeBounds(worldPartBounds);
}

std::string nextCubeName(const std::vector<RenderObject>& objects) {
    std::unordered_set<std::string> names;
    names.reserve(objects.size());
    for (const RenderObject& object : objects) {
        names.insert(object.name);
    }
    for (std::size_t index = 0; index <= objects.size(); ++index) {
        std::string candidate = "Cube " + std::to_string(index);
        if (!names.contains(candidate)) {
            return candidate;
        }
    }
    return "Cube";
}

std::string duplicateObjectName(const std::string& name) {
    constexpr const char* suffix = " copy";
    constexpr std::size_t suffixLength = 5;
    if (name.empty() ||
        name.size() + suffixLength > core::kMaxSceneObjectNameLength) {
        return "Object copy";
    }
    return name + suffix;
}

const char* shaderViewModeName(int mode) {
    switch (mode) {
        case 1: return "Albedo";
        case 2: return "Normals";
        case 3: return "Diffuse";
        case 4: return "Specular";
        default: return "Lit";
    }
}

std::string normalizedAssetReference(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

std::string materialIdForTexture(const std::string& texturePath) {
    return "texture:" + texturePath;
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
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

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

    const auto readGlString = [](GLenum name, const char* fallback) {
        const auto* value = glGetString(name);
        return value != nullptr ? std::string(reinterpret_cast<const char*>(value))
                                : std::string(fallback);
    };
    m_gpuVendor = readGlString(GL_VENDOR, "Unknown vendor");
    m_gpuRenderer = readGlString(GL_RENDERER, "Unknown GPU");
    m_openGlVersion = readGlString(GL_VERSION, "Unknown OpenGL version");

    std::cout << "OpenGL: " << m_openGlVersion << "\n";
    std::cout << "GPU:    " << m_gpuRenderer << " (" << m_gpuVendor << ")\n";
    std::cout << "Assets: " << core::findAssetsRoot().string() << "\n";

    // Enable debug output
    GL_EnableDebugOutput();

    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);

    // UI text
    UIText::init(m_width, m_height);

    // Camera
    m_camera = new Camera(glm::vec3(0.0f, 3.0f, 8.0f), -90.0f, -20.0f);

    // Mouse callbacks
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
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
    m_objects = {
        {"Cube 0", "builtin:cube", m_cubeMesh, nullptr, glm::vec3(-4.0f, 0.0f, 0.0f),
         glm::vec3(0.0f), glm::vec3(1.0f), matJager, false},
        {"Cube 1", "builtin:cube", m_cubeMesh, nullptr, glm::vec3(-2.0f, 0.0f, 0.0f),
         glm::vec3(0.0f), glm::vec3(1.0f), matZelya, false},
        {"Cube 2", "builtin:cube", m_cubeMesh, nullptr, glm::vec3(0.0f),
         glm::vec3(0.0f), glm::vec3(1.0f), matGrid, false},
        {"Cube 3", "builtin:cube", m_cubeMesh, nullptr, glm::vec3(2.0f, 0.0f, 0.0f),
         glm::vec3(0.0f), glm::vec3(1.0f), matJager, false},
        {"Cube 4", "builtin:cube", m_cubeMesh, nullptr, glm::vec3(4.0f, 0.0f, 0.0f),
         glm::vec3(0.0f), glm::vec3(1.0f), matZelya, false}
    };
    for (RenderObject& object : m_objects) {
        object.runtimeId = m_nextObjectId++;
    }

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

    m_coordinateGrid = std::make_unique<CoordinateGrid>();
    if (!m_coordinateGrid->init()) {
        std::cerr << "Failed to create coordinate grid\n";
        return false;
    }

    m_selectionOutline = std::make_unique<SelectionOutline>();
    if (!m_selectionOutline->init()) {
        std::cerr << "Selection outline is unavailable\n";
        m_selectionOutline.reset();
    }

    m_sceneFramebuffer = std::make_unique<Framebuffer>();
    if (!m_sceneFramebuffer->init(m_width, m_height)) {
        std::cerr << "Failed to create scene framebuffer\n";
        return false;
    }

    m_postProcessor = std::make_unique<PostProcessor>();
    if (!m_postProcessor->init()) {
        std::cerr << "Failed to initialize post processor\n";
        return false;
    }

    m_benchmarkFramebuffer = std::make_unique<Framebuffer>();
    if (!m_benchmarkFramebuffer->init(kBenchmarkWidth, kBenchmarkHeight)) {
        std::cerr << "Failed to create benchmark framebuffer\n";
        return false;
    }
    
    // Texture menu
    Menu::init();
    
    // Sync initial light position with menu
    Menu::setLightPosition(m_pointLight.position.x, m_pointLight.position.y,
                           m_pointLight.position.z);
    syncSelectedObjectToMenu();

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

bool Application::ensureSceneFramebuffer(int width, int height) {
    if (!m_sceneFramebuffer || width <= 0 || height <= 0) {
        return false;
    }
    if (m_sceneFramebuffer->isValid() &&
        m_sceneFramebuffer->width() == width && m_sceneFramebuffer->height() == height) {
        return true;
    }
    if (!m_sceneFramebuffer->init(width, height)) {
        std::cerr << "Failed to resize scene framebuffer to "
                  << width << "x" << height << "\n";
        return false;
    }
    return true;
}

Material* Application::getOrCreateMaterial(const std::string& relativeTexturePath) {
    if (!m_shader || relativeTexturePath.empty()) {
        return nullptr;
    }

    const std::string cacheKey = normalizedAssetReference(relativeTexturePath);
    const std::string materialId = materialIdForTexture(cacheKey);

    auto existing = m_materials.find(materialId);
    if (existing != m_materials.end()) {
        return existing->second.get();
    }

    std::shared_ptr<Texture2D> texture = ResourceManager::getTexture(cacheKey);
    if (!texture) {
        return nullptr;
    }

    auto material = std::make_unique<Material>(
        m_shader.get(), texture.get(), materialId, cacheKey);
    Material* materialPtr = material.get();

    m_textures.emplace(cacheKey, std::move(texture));
    m_materials.emplace(materialId, std::move(material));
    return materialPtr;
}

std::shared_ptr<Model> Application::loadModelAsset(
    const std::string& assetReference,
    MaterialCache& materials,
    TextureCache& textures,
    ModelCache& models,
    std::string& error) {
    const std::string normalizedReference = normalizedAssetReference(assetReference);
    if (const auto existing = models.find(normalizedReference); existing != models.end()) {
        return existing->second;
    }

    ModelLoadResult loaded = ModelLoader::load(normalizedReference);
    if (!loaded.success()) {
        error = loaded.error;
        return nullptr;
    }

    for (const ImportedMaterial& imported : loaded.materials) {
        if (materials.contains(imported.id)) {
            continue;
        }

        std::shared_ptr<Texture2D> texture;
        if (!imported.albedoTexture.empty()) {
            if (const auto existingTexture = textures.find(imported.albedoTexture);
                existingTexture != textures.end()) {
                texture = existingTexture->second;
            } else {
                texture = ResourceManager::getTexture(imported.albedoTexture);
                if (texture) {
                    textures.emplace(imported.albedoTexture, texture);
                } else {
                    loaded.warnings.push_back(
                        "failed to load material texture; using color fallback: " +
                        imported.albedoTexture);
                }
            }
        }

        auto material = std::make_unique<Material>(
            m_shader.get(), texture.get(), imported.id, imported.albedoTexture);
        material->albedoColor = imported.albedoColor;
        material->specularColor = imported.specularColor;
        material->emissiveColor = imported.emissiveColor;
        material->shininess = imported.shininess;
        material->metallic = imported.metallic;
        material->roughness = imported.roughness;
        materials.emplace(imported.id, std::move(material));
    }

    for (const std::string& warning : loaded.warnings) {
        std::cerr << "[ModelLoader] " << normalizedReference << ": " << warning << "\n";
    }
    std::shared_ptr<Model> model = std::move(loaded.model);
    models.emplace(normalizedReference, model);
    return model;
}

void Application::loadSelectedModel(const std::string& assetReference) {
    m_sceneMessageTime = 3.0f;
    if (m_selectedObject < 0 ||
        m_selectedObject >= static_cast<int>(m_objects.size())) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Model load failed: no selected object";
        return;
    }

    std::string error;
    std::shared_ptr<Model> model = loadModelAsset(
        assetReference, m_materials, m_textures, m_models, error);
    if (!model) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Model load failed";
        std::cerr << "Failed to load model " << assetReference << ": " << error << "\n";
        return;
    }

    RenderObject& object = m_objects[static_cast<std::size_t>(m_selectedObject)];
    const std::string normalizedReference = normalizedAssetReference(assetReference);
    if (object.model == model && object.meshAsset == normalizedReference) {
        m_sceneOperationSucceeded = true;
        m_sceneMessage = "Model already loaded: " +
                         std::filesystem::path(assetReference).filename().string();
        return;
    }

    checkpointScene();
    object.meshAsset = normalizedReference;
    object.mesh.reset();
    object.model = std::move(model);
    object.material = nullptr;
    m_sceneOperationSucceeded = true;
    m_sceneMessage = "Model loaded: " + std::filesystem::path(assetReference).filename().string();
    std::cout << m_sceneMessage << " | vertices: " << object.model->vertexCount()
              << " | indices: " << object.model->indexCount() << "\n";
}

void Application::selectObject(int direction) {
    if (m_objects.empty()) {
        m_selectedObject = -1;
        syncSelectedObjectToMenu();
        return;
    }

    const int objectCount = static_cast<int>(m_objects.size());
    if (m_selectedObject < 0 || m_selectedObject >= objectCount) {
        m_selectedObject = direction < 0 ? objectCount - 1 : 0;
    } else {
        m_selectedObject = (m_selectedObject + (direction < 0 ? -1 : 1) +
                            objectCount) % objectCount;
    }
    syncSelectedObjectToMenu();
}

void Application::selectObjectUnderCrosshair() {
    m_sceneMessageTime = 1.5f;
    if (m_camera == nullptr || m_objects.empty()) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Selection failed: no scene objects";
        return;
    }

    const float directionLength = glm::length(m_camera->front);
    if (!std::isfinite(directionLength) || directionLength <= 0.000001f) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Selection failed: invalid camera direction";
        return;
    }

    const glm::vec3 direction = m_camera->front / directionLength;
    float nearestDistance = kCameraFarPlane;
    int nearestObject = -1;

    for (std::size_t objectIndex = 0; objectIndex < m_objects.size(); ++objectIndex) {
        const RenderObject& object = m_objects[objectIndex];
        const glm::mat4 transform = objectTransform(object);
        const auto testMesh = [&](const std::shared_ptr<Mesh>& mesh) {
            if (mesh == nullptr) {
                return;
            }
            const std::optional<float> hit = core::intersectRayTransformedIndexedMesh(
                m_camera->position, direction, mesh->bounds(),
                mesh->pickingPositions(), mesh->pickingIndices(), transform,
                kCameraNearPlane, nearestDistance);
            if (hit.has_value() && *hit < nearestDistance) {
                nearestDistance = *hit;
                nearestObject = static_cast<int>(objectIndex);
            }
        };

        if (object.model != nullptr) {
            for (const ModelPart& part : object.model->parts()) {
                testMesh(part.mesh);
            }
        } else {
            testMesh(object.mesh);
        }
    }

    if (nearestObject < 0) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "No object under crosshair";
        return;
    }

    m_selectedObject = nearestObject;
    syncSelectedObjectToMenu();
    m_sceneOperationSucceeded = true;
    m_sceneMessage = "Selected: " +
                     m_objects[static_cast<std::size_t>(m_selectedObject)].name;
}

void Application::createCubeObject() {
    m_sceneMessageTime = 2.0f;
    if (m_camera == nullptr || m_cubeMesh == nullptr) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Create failed: renderer is not ready";
        return;
    }
    if (m_objects.size() >= core::kMaxSceneObjectCount) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Create failed: object limit reached";
        return;
    }

    const float directionLength = glm::length(m_camera->front);
    if (!std::isfinite(directionLength) || directionLength <= 0.000001f) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Create failed: invalid camera direction";
        return;
    }

    const glm::vec3 direction = m_camera->front / directionLength;
    glm::vec3 basePosition = m_camera->position + direction * 5.0f;
    if (std::abs(direction.y) > 0.000001f) {
        const float groundDistance = -m_camera->position.y / direction.y;
        if (std::isfinite(groundDistance) &&
            groundDistance >= 1.0f && groundDistance <= 50.0f) {
            basePosition = m_camera->position + direction * groundDistance;
        }
    }
    basePosition.x = std::round(basePosition.x);
    basePosition.y = 0.0f;
    basePosition.z = std::round(basePosition.z);

    std::vector<geometry::AxisAlignedBounds> occupiedBounds;
    occupiedBounds.reserve(m_objects.size());
    for (const RenderObject& object : m_objects) {
        if (const auto bounds = objectWorldBounds(object); bounds.has_value()) {
            occupiedBounds.push_back(*bounds);
        }
    }
    const std::optional<glm::vec3> position =
        core::findNearestFreeCubeGridPosition(
            basePosition, occupiedBounds, core::kMaxSceneCoordinate - 0.5f);
    if (!position.has_value()) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Create failed: no free position nearby";
        return;
    }

    core::SceneDocument sceneBeforeCreation = captureScene();
    const std::string defaultMaterialId =
        materialIdForTexture("textures/generated_grid");
    const bool defaultMaterialAlreadyUsed = std::any_of(
        sceneBeforeCreation.materials.begin(), sceneBeforeCreation.materials.end(),
        [&defaultMaterialId](const core::SceneMaterial& material) {
            return material.id == defaultMaterialId;
        });
    if (sceneBeforeCreation.materials.size() >= core::kMaxSceneMaterialCount &&
        !defaultMaterialAlreadyUsed) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Create failed: material limit reached";
        return;
    }

    Material* material = getOrCreateMaterial("textures/generated_grid");
    if (material == nullptr) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Create failed: default material is unavailable";
        return;
    }

    m_sceneHistory.record(std::move(sceneBeforeCreation));
    RenderObject object{
        nextCubeName(m_objects), "builtin:cube", m_cubeMesh, nullptr, *position,
        glm::vec3(0.0f), glm::vec3(1.0f), material, false, m_nextObjectId++
    };
    m_objects.push_back(std::move(object));
    m_selectedObject = static_cast<int>(m_objects.size()) - 1;
    syncSelectedObjectToMenu();

    m_sceneOperationSucceeded = true;
    m_sceneMessage = "Cube created: " +
                     m_objects[static_cast<std::size_t>(m_selectedObject)].name;
}

void Application::focusSelectedObject() {
    m_sceneMessageTime = 2.0f;
    if (m_camera == nullptr || m_selectedObject < 0 ||
        m_selectedObject >= static_cast<int>(m_objects.size())) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Focus failed: no selected object";
        return;
    }

    const RenderObject& selected =
        m_objects[static_cast<std::size_t>(m_selectedObject)];
    const std::optional<geometry::AxisAlignedBounds> worldBounds =
        objectWorldBounds(selected);
    const float aspectRatio = m_height > 0
        ? static_cast<float>(m_width) / static_cast<float>(m_height)
        : 1.0f;
    const std::optional<glm::vec3> framedPosition = worldBounds.has_value()
        ? core::calculateFramedCameraPosition(
              *worldBounds, m_camera->front, m_camera->fov, aspectRatio,
              kCameraNearPlane, kCameraFarPlane)
        : std::nullopt;
    if (!framedPosition.has_value() ||
        std::abs(framedPosition->x) > core::kMaxSceneCoordinate ||
        std::abs(framedPosition->y) > core::kMaxSceneCoordinate ||
        std::abs(framedPosition->z) > core::kMaxSceneCoordinate) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Focus failed: object cannot fit in the camera range";
        return;
    }

    m_camera->position = *framedPosition;
    m_firstMouse = true;
    m_sceneOperationSucceeded = true;
    m_sceneMessage = "Focused: " + selected.name;
}

void Application::duplicateSelectedObject() {
    m_sceneMessageTime = 2.0f;
    if (m_selectedObject < 0 ||
        m_selectedObject >= static_cast<int>(m_objects.size())) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Duplicate failed: no selected object";
        return;
    }
    if (m_objects.size() >= core::kMaxSceneObjectCount) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Duplicate failed: object limit reached";
        return;
    }

    checkpointScene();
    RenderObject duplicate = m_objects[static_cast<std::size_t>(m_selectedObject)];
    duplicate.name = duplicateObjectName(duplicate.name);
    duplicate.position += glm::vec3(0.75f, 0.0f, 0.75f);
    duplicate.runtimeId = m_nextObjectId++;

    const auto insertion = m_objects.begin() + m_selectedObject + 1;
    m_objects.insert(insertion, std::move(duplicate));
    ++m_selectedObject;
    syncSelectedObjectToMenu();

    m_sceneOperationSucceeded = true;
    m_sceneMessage = "Object duplicated: " +
                     m_objects[static_cast<std::size_t>(m_selectedObject)].name;
}

void Application::deleteSelectedObject() {
    m_sceneMessageTime = 2.0f;
    if (m_selectedObject < 0 ||
        m_selectedObject >= static_cast<int>(m_objects.size())) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = "Delete failed: no selected object";
        return;
    }

    checkpointScene();
    const std::string deletedName =
        m_objects[static_cast<std::size_t>(m_selectedObject)].name;
    m_objects.erase(m_objects.begin() + m_selectedObject);
    if (m_objects.empty()) {
        m_selectedObject = -1;
    } else if (m_selectedObject >= static_cast<int>(m_objects.size())) {
        m_selectedObject = static_cast<int>(m_objects.size()) - 1;
    }
    syncSelectedObjectToMenu();

    m_sceneOperationSucceeded = true;
    m_sceneMessage = "Object deleted: " + deletedName;
}

void Application::syncSelectedObjectToMenu() {
    if (m_selectedObject < 0 ||
        m_selectedObject >= static_cast<int>(m_objects.size())) {
        m_selectedObject = m_objects.empty() ? -1 : 0;
    }

    Menu::setSelectedCubeIndex(m_selectedObject);
    if (m_selectedObject >= 0) {
        const RenderObject& selected =
            m_objects[static_cast<std::size_t>(m_selectedObject)];
        Menu::setCubePosition(
            selected.position.x, selected.position.y, selected.position.z);
    } else {
        Menu::setCubePosition(0.0f, 0.0f, 0.0f);
    }
}

void Application::checkpointScene() {
    m_sceneHistory.record(captureScene());
}

void Application::restoreSceneHistory(bool redo) {
    m_sceneMessageTime = 2.0f;
    const core::SceneDocument* target =
        redo ? m_sceneHistory.redoTarget() : m_sceneHistory.undoTarget();
    if (target == nullptr) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = redo ? "Nothing to redo" : "Nothing to undo";
        return;
    }

    core::SceneDocument current = captureScene();
    core::SceneDocument restored = *target;
    restored.camera = current.camera;
    restored.selectedObject =
        core::resolveSceneHistorySelection(current, restored);

    std::string error;
    if (!applyScene(restored, error)) {
        m_sceneOperationSucceeded = false;
        m_sceneMessage = redo ? "Redo failed" : "Undo failed";
        std::cerr << m_sceneMessage << ": " << error << "\n";
        return;
    }

    const bool committed = redo
        ? m_sceneHistory.commitRedo(std::move(current))
        : m_sceneHistory.commitUndo(std::move(current));
    m_sceneOperationSucceeded = committed;
    m_sceneMessage = committed
        ? (redo ? "Scene change redone" : "Scene change undone")
        : (redo ? "Redo failed" : "Undo failed");
}

Material* Application::selectedMaterial() {
    if (m_selectedObject < 0 ||
        m_selectedObject >= static_cast<int>(m_objects.size())) {
        return nullptr;
    }
    const RenderObject& object = m_objects[static_cast<std::size_t>(m_selectedObject)];
    if (object.material != nullptr) {
        return object.material;
    }
    if (object.model != nullptr && !object.model->parts().empty()) {
        const auto material = m_materials.find(object.model->parts().front().materialId);
        if (material != m_materials.end()) {
            return material->second.get();
        }
    }
    return nullptr;
}

const Material* Application::selectedMaterial() const {
    if (m_selectedObject < 0 ||
        m_selectedObject >= static_cast<int>(m_objects.size())) {
        return nullptr;
    }
    const RenderObject& object = m_objects[static_cast<std::size_t>(m_selectedObject)];
    if (object.material != nullptr) {
        return object.material;
    }
    if (object.model != nullptr && !object.model->parts().empty()) {
        const auto material = m_materials.find(object.model->parts().front().materialId);
        if (material != m_materials.end()) {
            return material->second.get();
        }
    }
    return nullptr;
}

core::SceneDocument Application::captureScene() const {
    core::SceneDocument scene;
    if (m_camera != nullptr) {
        scene.camera.position = m_camera->position;
        scene.camera.yaw = m_camera->yaw;
        scene.camera.pitch = m_camera->pitch;
        scene.camera.fov = m_camera->fov;
        scene.camera.movementSpeed = m_camera->movementSpeed;
        scene.camera.mouseSensitivity = m_camera->mouseSensitivity;
    }

    scene.directionalLight.direction = m_dirLight.direction;
    scene.directionalLight.color = m_dirLight.color;
    scene.directionalLight.enabled = m_dirLight.enabled;
    scene.pointLight.position = m_pointLight.position;
    scene.pointLight.color = m_pointLight.color;
    scene.pointLight.constant = m_pointLight.constant;
    scene.pointLight.linear = m_pointLight.linear;
    scene.pointLight.quadratic = m_pointLight.quadratic;
    scene.pointLight.enabled = m_pointLight.enabled;
    scene.pointLight.spinning = m_pointLightSpinning;
    scene.renderSettings.postProcessEffect = static_cast<int>(m_postProcessEffect);
    scene.renderSettings.shaderViewMode = m_shaderViewMode;
    scene.renderSettings.coordinateGrid = m_showCoordinateGrid;
    scene.selectedObject = m_objects.empty() ? -1 : m_selectedObject;

    std::unordered_set<std::string> usedMaterialIds;
    for (const RenderObject& object : m_objects) {
        if (object.material != nullptr) {
            usedMaterialIds.insert(object.material->id);
        } else if (object.model != nullptr) {
            for (const ModelPart& part : object.model->parts()) {
                usedMaterialIds.insert(part.materialId);
            }
        }
    }

    std::vector<std::string> materialIds(
        usedMaterialIds.begin(), usedMaterialIds.end());
    std::sort(materialIds.begin(), materialIds.end());
    scene.materials.reserve(materialIds.size());
    for (const std::string& id : materialIds) {
        const Material& material = *m_materials.at(id);
        core::SceneMaterial saved;
        saved.id = id;
        saved.albedoTexture = material.albedoTexturePath;
        saved.albedoColor = material.albedoColor;
        saved.specularColor = material.specularColor;
        saved.emissiveColor = material.emissiveColor;
        saved.shininess = material.shininess;
        saved.metallic = material.metallic;
        saved.roughness = material.roughness;
        scene.materials.push_back(std::move(saved));
    }

    scene.objects.reserve(m_objects.size());
    for (const RenderObject& object : m_objects) {
        core::SceneObject saved;
        saved.name = object.name;
        saved.mesh = object.meshAsset.empty() ? "builtin:cube" : object.meshAsset;
        saved.material = object.material != nullptr ? object.material->id : std::string();
        saved.position = object.position;
        saved.rotationDeg = object.rotationDeg;
        saved.scale = object.scale;
        saved.spinning = object.spinning;
        saved.runtimeId = object.runtimeId;
        scene.objects.push_back(std::move(saved));
    }
    return scene;
}

bool Application::applyScene(const core::SceneDocument& scene, std::string& error) {
    if (!m_shader || !m_cubeMesh || m_camera == nullptr) {
        error = "renderer resources are not ready";
        return false;
    }
    if (!core::validateSceneDocument(scene, error)) {
        return false;
    }
    MaterialCache newMaterials;
    TextureCache newTextures;
    ModelCache newModels;
    newMaterials.reserve(scene.materials.size());
    newTextures.reserve(scene.materials.size());
    for (const core::SceneMaterial& saved : scene.materials) {
        std::shared_ptr<Texture2D> texture;
        if (!saved.albedoTexture.empty()) {
            texture = ResourceManager::getTexture(saved.albedoTexture);
            if (!texture) {
                error = "failed to load texture: " + saved.albedoTexture;
                return false;
            }
            newTextures.emplace(saved.albedoTexture, texture);
        }

        auto material = std::make_unique<Material>(
            m_shader.get(), texture.get(), saved.id, saved.albedoTexture);
        material->albedoColor = saved.albedoColor;
        material->specularColor = saved.specularColor;
        material->emissiveColor = saved.emissiveColor;
        material->shininess = saved.shininess;
        material->metallic = saved.metallic;
        material->roughness = saved.roughness;
        newMaterials.emplace(saved.id, std::move(material));
    }

    std::vector<RenderObject> newObjects;
    newObjects.reserve(scene.objects.size());
    for (const core::SceneObject& saved : scene.objects) {
        Material* materialOverride = nullptr;
        if (!saved.material.empty()) {
            const auto material = newMaterials.find(saved.material);
            if (material == newMaterials.end()) {
                error = "object references a missing material: " + saved.material;
                return false;
            }
            materialOverride = material->second.get();
        }

        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Model> model;
        if (saved.mesh == "builtin:cube") {
            if (materialOverride == nullptr) {
                error = "built-in cube requires a material";
                return false;
            }
            mesh = m_cubeMesh;
        } else {
            model = loadModelAsset(
                saved.mesh, newMaterials, newTextures, newModels, error);
            if (!model) {
                error = "failed to load " + saved.mesh + ": " + error;
                return false;
            }
        }
        std::uint64_t runtimeId = saved.runtimeId;
        if (runtimeId == 0) {
            runtimeId = m_nextObjectId++;
        } else if (runtimeId >= m_nextObjectId &&
                   runtimeId != (std::numeric_limits<std::uint64_t>::max)()) {
            m_nextObjectId = runtimeId + 1;
        }
        newObjects.push_back({
            saved.name, saved.mesh, std::move(mesh), std::move(model), saved.position,
            saved.rotationDeg, saved.scale, materialOverride, saved.spinning, runtimeId
        });
    }

    Camera loadedCamera(scene.camera.position, scene.camera.yaw, scene.camera.pitch);
    loadedCamera.fov = scene.camera.fov;
    loadedCamera.movementSpeed = scene.camera.movementSpeed;
    loadedCamera.mouseSensitivity = scene.camera.mouseSensitivity;

    m_objects = std::move(newObjects);
    m_materials = std::move(newMaterials);
    m_textures = std::move(newTextures);
    m_models = std::move(newModels);
    *m_camera = loadedCamera;
    m_dirLight.direction = glm::normalize(scene.directionalLight.direction);
    m_dirLight.color = scene.directionalLight.color;
    m_dirLight.enabled = scene.directionalLight.enabled;
    m_pointLight.position = scene.pointLight.position;
    m_pointLight.color = scene.pointLight.color;
    m_pointLight.constant = scene.pointLight.constant;
    m_pointLight.linear = scene.pointLight.linear;
    m_pointLight.quadratic = scene.pointLight.quadratic;
    m_pointLight.enabled = scene.pointLight.enabled;
    m_pointLightSpinning = scene.pointLight.spinning;
    m_pointLightSpinAngle = std::atan2(m_pointLight.position.z, m_pointLight.position.x);
    m_postProcessEffect = static_cast<PostProcessEffect>(
        scene.renderSettings.postProcessEffect);
    m_shaderViewMode = scene.renderSettings.shaderViewMode;
    m_showCoordinateGrid = scene.renderSettings.coordinateGrid;
    m_selectedObject = scene.selectedObject;
    m_firstMouse = true;

    Menu::setLightPosition(
        m_pointLight.position.x, m_pointLight.position.y, m_pointLight.position.z);
    syncSelectedObjectToMenu();
    resetFrameStatistics();
    return true;
}

void Application::saveQuickScene() {
    const std::filesystem::path path = core::quickSaveScenePath();
    const core::SceneIoResult result = core::saveSceneDocument(captureScene(), path);
    m_sceneOperationSucceeded = result.success;
    m_sceneMessageTime = 3.0f;
    if (result.success) {
        m_sceneMessage = "Scene saved: " + path.filename().string();
        std::cout << "Scene saved: " << path.string() << "\n";
    } else {
        m_sceneMessage = "Scene save failed";
        std::cerr << "Failed to save scene: " << result.error << "\n";
    }
}

void Application::loadQuickScene() {
    const std::filesystem::path path = core::quickSaveScenePath();
    core::SceneDocument scene;
    core::SceneIoResult result = core::loadSceneDocument(path, scene);
    core::SceneDocument previousScene;
    std::string applyError;
    if (result.success) {
        previousScene = captureScene();
        if (!applyScene(scene, applyError)) {
            result.success = false;
            result.error = std::move(applyError);
        } else {
            m_sceneHistory.record(std::move(previousScene));
        }
    }

    m_sceneOperationSucceeded = result.success;
    m_sceneMessageTime = 3.0f;
    if (result.success) {
        m_sceneMessage = "Scene loaded: " + path.filename().string();
        std::cout << "Scene loaded: " << path.string() << "\n";
    } else {
        m_sceneMessage = "Scene load failed";
        std::cerr << "Failed to load scene: " << result.error << "\n";
    }
}

void Application::resetFrameStatistics() {
    m_fpsUpdateTime = 0.0f;
    m_frameCount = 0;
    m_currentFPS = 0.0f;
    m_cpuFrameTimeMs = 0.0f;
    m_cpuWorkTimeMs = 0.0f;
    m_presentTimeMs = 0.0f;
    m_hasCpuWorkTime = false;
    m_hasFrameStatistics = false;
    m_hasPresentTime = false;
    m_skipNextFrameSample = true;
    m_lastFrame = static_cast<float>(glfwGetTime());

    if (m_renderer != nullptr) {
        m_renderer->resetGpuFrameTimes();
    }
}

void Application::restartBenchmarkCapture() {
    if (!m_benchmarkEnabled || m_renderer == nullptr) {
        m_benchmarkCaptureState = BenchmarkCaptureState::Idle;
        return;
    }

    m_renderer->cancelGpuBenchmarkCapture();
    m_benchmarkCaptureState = BenchmarkCaptureState::WarmingUp;
    m_benchmarkWarmupStart = glfwGetTime();
    m_benchmarkMedianGpuMs = 0.0f;
    m_benchmarkP95GpuMs = 0.0f;
    m_benchmarkReportStatus.clear();
    m_benchmarkComparisonStatus.clear();
    m_benchmarkCpuCaptureActive = false;
    m_benchmarkFrameIntervalSamples.clear();
    m_benchmarkCpuWorkSamples.clear();
    m_benchmarkPresentSamples.clear();
}

void Application::recordBenchmarkFrameSample(float frameIntervalMs, float cpuWorkMs,
                                             float presentMs) {
    if (!m_benchmarkCpuCaptureActive ||
        m_benchmarkFrameIntervalSamples.size() >= kBenchmarkCaptureSamples) {
        return;
    }

    m_benchmarkFrameIntervalSamples.push_back(frameIntervalMs);
    m_benchmarkCpuWorkSamples.push_back(cpuWorkMs);
    m_benchmarkPresentSamples.push_back(presentMs);
    if (m_benchmarkFrameIntervalSamples.size() >= kBenchmarkCaptureSamples) {
        m_benchmarkCpuCaptureActive = false;
    }
}

void Application::updateBenchmarkCapture() {
    if (!m_benchmarkEnabled || m_renderer == nullptr) {
        return;
    }

    if (m_benchmarkCaptureState == BenchmarkCaptureState::WarmingUp) {
        if (glfwGetTime() - m_benchmarkWarmupStart >= kBenchmarkWarmupSeconds) {
            m_renderer->beginGpuBenchmarkCapture(kBenchmarkCaptureSamples);
            m_benchmarkFrameIntervalSamples.clear();
            m_benchmarkCpuWorkSamples.clear();
            m_benchmarkPresentSamples.clear();
            m_benchmarkFrameIntervalSamples.reserve(kBenchmarkCaptureSamples);
            m_benchmarkCpuWorkSamples.reserve(kBenchmarkCaptureSamples);
            m_benchmarkPresentSamples.reserve(kBenchmarkCaptureSamples);
            m_benchmarkCpuCaptureActive = true;
            m_benchmarkCaptureState = BenchmarkCaptureState::Capturing;
        }
        return;
    }

    if (m_benchmarkCaptureState != BenchmarkCaptureState::Capturing ||
        m_renderer->gpuBenchmarkCaptureActive()) {
        return;
    }

    const auto& gpuSamples = m_renderer->gpuBenchmarkCaptureSamples();
    if (gpuSamples.size() < kBenchmarkCaptureSamples ||
        m_benchmarkFrameIntervalSamples.size() < kBenchmarkCaptureSamples ||
        m_benchmarkCpuWorkSamples.size() < kBenchmarkCaptureSamples ||
        m_benchmarkPresentSamples.size() < kBenchmarkCaptureSamples) {
        return;
    }

    const std::size_t sampleCount = gpuSamples.size();
    std::vector<float> drawSamples;
    std::vector<float> gpuTotalSamples;
    std::vector<float> gpuSceneSamples;
    std::vector<float> gpuUiSamples;
    drawSamples.reserve(sampleCount);
    gpuTotalSamples.reserve(sampleCount);
    gpuSceneSamples.reserve(sampleCount);
    gpuUiSamples.reserve(sampleCount);
    for (const GpuBenchmarkSample& sample : gpuSamples) {
        drawSamples.push_back(sample.drawMs);
        gpuTotalSamples.push_back(sample.totalMs);
        gpuSceneSamples.push_back(sample.sceneMs);
        gpuUiSamples.push_back(sample.uiMs);
    }

    core::BenchmarkReport report;
    report.workload = "fixed_instanced_shader_v1";
    report.gpuVendor = m_gpuVendor;
    report.gpuRenderer = m_gpuRenderer;
    report.openGlVersion = m_openGlVersion;
    report.targetWidth = kBenchmarkWidth;
    report.targetHeight = kBenchmarkHeight;
    report.displayWidth = m_width;
    report.displayHeight = m_height;
    report.instances = m_benchmarkInstanceCount;
    report.shaderIterations = kBenchmarkShaderIterations;
    report.warmupSeconds = kBenchmarkWarmupSeconds;
    report.draw = core::makeBenchmarkMetric(std::move(drawSamples));
    report.frameInterval = core::makeBenchmarkMetric(m_benchmarkFrameIntervalSamples);
    report.cpuWork = core::makeBenchmarkMetric(m_benchmarkCpuWorkSamples);
    report.present = core::makeBenchmarkMetric(m_benchmarkPresentSamples);
    report.gpuTotal = core::makeBenchmarkMetric(std::move(gpuTotalSamples));
    report.gpuScene = core::makeBenchmarkMetric(std::move(gpuSceneSamples));
    report.gpuUi = core::makeBenchmarkMetric(std::move(gpuUiSamples));
    m_benchmarkMedianGpuMs = static_cast<float>(report.draw.statistics.medianMs);
    m_benchmarkP95GpuMs = static_cast<float>(report.draw.statistics.p95Ms);

    const std::filesystem::path reportDirectory = core::benchmarkResultsDir();
    const core::BenchmarkReportResult reportResult =
        core::writeBenchmarkReport(report, reportDirectory);

    if (reportResult.success) {
        m_benchmarkReportStatus = "Report: " + reportResult.jsonPath.filename().string();
        std::cout << "Benchmark JSON: " << reportResult.jsonPath.string() << "\n";
        std::cout << "Benchmark CSV:  " << reportResult.csvPath.string() << "\n";

        const auto comparison = core::findLatestCompatibleGpuComparison(report, reportResult);
        if (comparison) {
            std::string comparisonGpu = comparison->gpuRenderer;
            if (comparisonGpu.length() > 32) {
                comparisonGpu = comparisonGpu.substr(0, 29) + "...";
            }
            const double baselineDraw = comparison->draw.medianMs;
            const double currentDraw = report.draw.statistics.medianMs;
            const double differencePercent = baselineDraw > 0.0
                ? std::abs(currentDraw / baselineDraw - 1.0) * 100.0
                : 0.0;
            const char* relation = differencePercent < 1.0
                ? "similar"
                : (currentDraw < baselineDraw ? "faster" : "slower");

            std::ostringstream comparisonText;
            comparisonText << std::fixed << std::setprecision(2)
                           << "vs " << comparisonGpu
                           << "\nDraw: " << currentDraw << " vs " << baselineDraw
                           << " ms (" << std::setprecision(1) << differencePercent
                           << "% " << relation << ")"
                           << std::setprecision(2)
                           << "\nFrame: " << report.frameInterval.statistics.medianMs
                           << " vs " << comparison->frameInterval.medianMs
                           << " | Present: " << report.present.statistics.medianMs
                           << " vs " << comparison->present.medianMs;
            m_benchmarkComparisonStatus = comparisonText.str();
            std::cout << "GPU comparison | " << m_benchmarkComparisonStatus << "\n";
        } else {
            m_benchmarkComparisonStatus = "Compare: run F7 on another GPU";
        }
    } else {
        m_benchmarkReportStatus = "Report failed (see console)";
        m_benchmarkComparisonStatus.clear();
        std::cerr << "Failed to save benchmark report: " << reportResult.error << "\n";
    }
    m_benchmarkCpuCaptureActive = false;
    m_benchmarkCaptureState = BenchmarkCaptureState::Complete;

    std::ostringstream result;
    result << std::fixed << std::setprecision(2)
           << "GPU benchmark result | GPU: " << m_gpuRenderer
           << " | Target: " << kBenchmarkWidth << "x" << kBenchmarkHeight
           << " | Samples: " << sampleCount
           << " | Draw median: " << m_benchmarkMedianGpuMs << " ms"
           << " | p95: " << m_benchmarkP95GpuMs << " ms"
           << " | Frame median: " << report.frameInterval.statistics.medianMs << " ms"
           << " | CPU work: " << report.cpuWork.statistics.medianMs << " ms"
           << " | Present: " << report.present.statistics.medianMs << " ms";
    std::cout << result.str() << "\n";
}

void Application::toggleBenchmark() {
    m_benchmarkEnabled = !m_benchmarkEnabled;
    if (m_benchmarkEnabled && Menu::isOpen()) {
        Menu::toggle();
    }
    m_firstMouse = true;
    resetFrameStatistics();
    if (m_benchmarkEnabled) {
        restartBenchmarkCapture();
    } else {
        m_benchmarkCpuCaptureActive = false;
        m_benchmarkCaptureState = BenchmarkCaptureState::Idle;
    }

    std::cout << "GPU benchmark " << (m_benchmarkEnabled ? "enabled" : "disabled") << "\n";
}

void Application::shutdown() {
    m_initialized = false;

    if (m_contextReady && m_window) {
        glfwMakeContextCurrent(m_window);
    }

    m_postProcessor.reset();
    m_selectionOutline.reset();
    m_coordinateGrid.reset();
    m_benchmarkFramebuffer.reset();
    m_sceneFramebuffer.reset();
    m_benchmarkInstanceCount = 0;
    m_benchmarkMaterial.reset();

    m_objects.clear();
    m_sceneHistory.clear();
    m_models.clear();
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
    m_gpuVendor.clear();
    m_gpuRenderer.clear();
    m_openGlVersion.clear();

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
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

        bool restoredFromMinimize = false;
        while ((framebufferWidth <= 0 || framebufferHeight <= 0) &&
               !glfwWindowShouldClose(m_window)) {
            restoredFromMinimize = true;
            glfwWaitEvents();
            glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
        }
        if (glfwWindowShouldClose(m_window)) {
            break;
        }

        // Delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        if (restoredFromMinimize) {
            m_lastFrame = currentFrame;
            m_skipNextFrameSample = true;
        }
        m_deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        const double cpuWorkStart = glfwGetTime();
        processInput(m_deltaTime);
        update(m_deltaTime);
        render();
        
        m_renderer->endFrame();
        const float cpuWorkSampleMs = static_cast<float>(
            (glfwGetTime() - cpuWorkStart) * 1000.0);
        if (m_hasCpuWorkTime) {
            constexpr float smoothingFactor = 0.1f;
            m_cpuWorkTimeMs += (cpuWorkSampleMs - m_cpuWorkTimeMs) * smoothingFactor;
        } else {
            m_cpuWorkTimeMs = cpuWorkSampleMs;
            m_hasCpuWorkTime = true;
        }

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

        if (!restoredFromMinimize) {
            recordBenchmarkFrameSample(
                m_deltaTime * 1000.0f, cpuWorkSampleMs, presentSampleMs);
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
        return;
    }
    
    // Keep menu navigation from moving the camera behind the overlay.
    if (!Menu::isOpen()) {
        const bool controlPressed =
            glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        const bool shiftPressed =
            glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        const bool duplicateShortcutHeld = controlPressed &&
            glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS;
        const bool historyShortcutHeld = controlPressed &&
            (glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_PRESS ||
             glfwGetKey(m_window, GLFW_KEY_Y) == GLFW_PRESS);
        const bool selectionShortcutHeld =
            glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_PRESS;

        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
            m_camera->processKeyboard(FORWARD, dt);
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
            m_camera->processKeyboard(BACKWARD, dt);
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
            m_camera->processKeyboard(LEFT, dt);
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS &&
            !duplicateShortcutHeld)
            m_camera->processKeyboard(RIGHT, dt);
        if (shiftPressed && !selectionShortcutHeld && !historyShortcutHeld)
            m_camera->processKeyboard(UP, dt);
        if (controlPressed && !duplicateShortcutHeld && !historyShortcutHeld)
            m_camera->processKeyboard(DOWN, dt);
    }
}

void Application::update(float dt) {
    if (m_reloadMessageTime > 0.0f) {
        m_reloadMessageTime -= dt;
    }
    if (m_sceneMessageTime > 0.0f) {
        m_sceneMessageTime -= dt;
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
            const bool changesObject = action != Menu::CUBE_NONE &&
                action != Menu::CUBE_PREV && action != Menu::CUBE_NEXT;
            if (changesObject) {
                checkpointScene();
            }
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
                if (obj.material != newMaterial) {
                    checkpointScene();
                    obj.material = newMaterial;
                }
            }
        }
        
        Menu::markReloaded();
    }

    if (Menu::needsModelLoad()) {
        loadSelectedModel(Menu::getSelectedModelPath());
        Menu::markModelLoaded();
    }
    
    if (Menu::needsLightUpdate()) {
        Menu::LightControlAction action = Menu::getLightControlAction();
        const float step = 0.5f;
        if (action != Menu::LIGHT_NONE) {
            checkpointScene();
        }
        
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
        if (action != Menu::DIRLIGHT_NONE) {
            checkpointScene();
        }
        
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
    updateBenchmarkCapture();
    
    if (m_shader != nullptr && m_shader->m_id != 0 && m_camera != nullptr) {
        if (m_benchmarkEnabled && m_cubeMesh && m_benchmarkMaterial != nullptr &&
            m_benchmarkFramebuffer && m_benchmarkFramebuffer->isValid()) {
            m_benchmarkFramebuffer->bind();
            glViewport(0, 0, m_benchmarkFramebuffer->width(), m_benchmarkFramebuffer->height());
            glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const glm::vec3 cameraPos(0.0f, 0.0f, 28.0f);
            const glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            const float benchmarkAspect = static_cast<float>(m_benchmarkFramebuffer->width()) /
                                          static_cast<float>(m_benchmarkFramebuffer->height());
            const glm::mat4 projection = glm::perspective(
                glm::radians(kBenchmarkFov), benchmarkAspect,
                kCameraNearPlane, kCameraFarPlane);
            DirectionalLight benchmarkDirLight;
            PointLight benchmarkPointLight;
            benchmarkPointLight.enabled = false;

            m_renderer->beginBenchmarkPass();
            m_renderer->drawMeshInstanced(*m_cubeMesh, *m_benchmarkMaterial,
                                          m_benchmarkInstanceCount, view, projection,
                                          cameraPos, benchmarkDirLight, benchmarkPointLight,
                                          kBenchmarkShaderIterations);
            m_renderer->endBenchmarkPass();

            Framebuffer::bindDefault();
            glViewport(0, 0, width, height);
            m_benchmarkFramebuffer->blitColorToDefault(width, height);
        } else {
            const bool usePostProcessing = m_postProcessor && m_postProcessor->isReady() &&
                                           ensureSceneFramebuffer(width, height);
            if (usePostProcessing) {
                m_sceneFramebuffer->bind();
                glViewport(0, 0, width, height);
                glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            const float aspect = static_cast<float>(width) / static_cast<float>(height);
            const glm::mat4 view = m_camera->getViewMatrix();
            const glm::mat4 projection = glm::perspective(
                glm::radians(m_camera->fov), aspect,
                kCameraNearPlane, kCameraFarPlane);

            if (m_showCoordinateGrid && m_coordinateGrid != nullptr) {
                m_coordinateGrid->draw(view, projection, m_camera->position);
            }

            const bool hasSelectedObject = m_selectedObject >= 0 &&
                m_selectedObject < static_cast<int>(m_objects.size());
            const bool outlinePassActive = hasSelectedObject &&
                m_selectionOutline != nullptr && m_selectionOutline->beginMask();

            for (std::size_t objectIndex = 0;
                 objectIndex < m_objects.size(); ++objectIndex) {
                const RenderObject& obj = m_objects[objectIndex];
                const glm::mat4 model = objectTransform(obj);
                if (outlinePassActive) {
                    m_selectionOutline->maskObject(
                        objectIndex == static_cast<std::size_t>(m_selectedObject));
                }

                if (obj.model != nullptr) {
                    for (const ModelPart& part : obj.model->parts()) {
                        const Material* material = obj.material;
                        if (material == nullptr) {
                            const auto importedMaterial = m_materials.find(part.materialId);
                            if (importedMaterial != m_materials.end()) {
                                material = importedMaterial->second.get();
                            }
                        }
                        if (part.mesh != nullptr && material != nullptr) {
                            m_renderer->drawMesh(
                                *part.mesh, *material, model, view, projection,
                                m_camera->position, m_dirLight, m_pointLight,
                                m_shaderViewMode);
                        }
                    }
                } else if (obj.mesh != nullptr && obj.material != nullptr) {
                    m_renderer->drawMesh(
                        *obj.mesh, *obj.material, model, view, projection,
                        m_camera->position, m_dirLight, m_pointLight,
                        m_shaderViewMode);
                }
            }

            if (outlinePassActive) {
                const RenderObject& selected =
                    m_objects[static_cast<std::size_t>(m_selectedObject)];
                const glm::mat4 selectedModel = objectTransform(selected);
                m_selectionOutline->beginOutline();
                if (selected.model != nullptr) {
                    for (const ModelPart& part : selected.model->parts()) {
                        if (part.mesh != nullptr) {
                            m_selectionOutline->drawMesh(
                                *part.mesh, selectedModel, view, projection);
                        }
                    }
                } else if (selected.mesh != nullptr) {
                    m_selectionOutline->drawMesh(
                        *selected.mesh, selectedModel, view, projection);
                }
                m_selectionOutline->end();
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

            if (usePostProcessing) {
                Framebuffer::bindDefault();
                glViewport(0, 0, width, height);
                m_postProcessor->render(
                    m_sceneFramebuffer->colorTexture(), m_postProcessEffect,
                    static_cast<float>(glfwGetTime()));
            }
        }
    }
    
    Menu::update();
    m_renderer->beginUiPass();
    UIText::beginFrame();
    Menu::render();

    if (!m_benchmarkEnabled && !Menu::isOpen()) {
        constexpr float crosshairScale = 1.5f;
        UIText::renderTextWithColor(
            "+",
            static_cast<float>(width) * 0.5f - 4.0f * crosshairScale,
            static_cast<float>(height) * 0.5f - 6.0f * crosshairScale,
            crosshairScale, 1.0f, 0.75f, 0.1f);
    }

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
    if (m_hasCpuWorkTime) {
        oss << std::setprecision(2);
        oss << "CPU work: " << m_cpuWorkTimeMs << " ms\n";
    } else {
        oss << "CPU work: warming up\n";
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
        if (m_benchmarkEnabled && m_renderer->hasGpuBenchmarkTime()) {
            oss << "  Draw: " << m_renderer->gpuBenchmarkTimeMs() << " ms\n";
        }
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
    if (!m_benchmarkEnabled) {
        oss << "\nGrid: " << (m_showCoordinateGrid ? "ON" : "OFF") << " [G]";
    }
    if (m_benchmarkEnabled) {
        oss << "\nTarget: " << kBenchmarkWidth << "x" << kBenchmarkHeight;
        oss << "\nDisplay: " << width << "x" << height;
        oss << "\nInstances: " << m_benchmarkInstanceCount;
        oss << "\nShader loops: " << kBenchmarkShaderIterations;
        oss << "\nCompare GPUs by Draw median";
    }
    if (m_showGPUInfo || m_benchmarkEnabled) {
        std::string gpuStr = m_gpuRenderer;
        if (gpuStr.length() > 40) {
            gpuStr = gpuStr.substr(0, 37) + "...";
        }
        oss << "\nGPU: " << gpuStr;
    }
    if (m_benchmarkEnabled && m_renderer != nullptr) {
        if (m_benchmarkCaptureState == BenchmarkCaptureState::WarmingUp) {
            const double elapsed = std::clamp(glfwGetTime() - m_benchmarkWarmupStart,
                                              0.0, kBenchmarkWarmupSeconds);
            oss << "\nWarm-up: " << std::setprecision(1) << elapsed
                << "/" << kBenchmarkWarmupSeconds << " s";
        } else if (m_benchmarkCaptureState == BenchmarkCaptureState::Capturing) {
            oss << "\nCapture: " << m_renderer->gpuBenchmarkCaptureSamples().size()
                << "/" << m_renderer->gpuBenchmarkCaptureTarget() << " samples";
        } else if (m_benchmarkCaptureState == BenchmarkCaptureState::Complete) {
            oss << std::setprecision(2);
            oss << "\nDraw median: " << m_benchmarkMedianGpuMs << " ms";
            oss << "\nDraw p95: " << m_benchmarkP95GpuMs << " ms";
            oss << "\nCapture complete [F10 rerun]";
            if (!m_benchmarkReportStatus.empty()) {
                oss << "\n" << m_benchmarkReportStatus;
            }
            if (!m_benchmarkComparisonStatus.empty()) {
                oss << "\n" << m_benchmarkComparisonStatus;
            }
        }
    }
    
    UIText::renderText(oss.str(), 10.0f, 10.0f, 1.5f);
    
    // Show shader reload status
    if (m_reloadMessageTime > 0.0f) {
        const float red = m_reloadSucceeded ? 0.0f : 1.0f;
        const float green = m_reloadSucceeded ? 1.0f : 0.0f;
        UIText::renderTextWithColor(m_reloadMessage, 10.0f, 200.0f, 1.5f, red, green, 0.0f);
    }
    if (m_sceneMessageTime > 0.0f) {
        const float red = m_sceneOperationSucceeded ? 0.0f : 1.0f;
        const float green = m_sceneOperationSucceeded ? 1.0f : 0.0f;
        UIText::renderTextWithColor(
            m_sceneMessage, 10.0f, 225.0f, 1.5f, red, green, 0.0f);
    }
    
    // Show active post-process and material inspection views.
    if (!m_benchmarkEnabled && m_shader != nullptr && m_shader->m_id != 0) {
        std::string shaderStatus = std::string("Post: ") +
                                   PostProcessor::effectName(m_postProcessEffect) +
                                   " [1-6]\nMaterial: " + shaderViewModeName(m_shaderViewMode) +
                                   " [F1-F4/F6]";
        UIText::renderTextWithColor(shaderStatus, 10.0f, 250.0f, 1.2f, 0.0f, 1.0f, 0.0f);
    }

    if (!m_benchmarkEnabled) {
        std::ostringstream lightOss;
        lightOss << std::fixed;
        if (const Material* material = selectedMaterial()) {
            std::string materialId = material->id;
            if (materialId.length() > 34) {
                materialId = materialId.substr(0, 31) + "...";
            }
            lightOss << "Material: " << materialId << "\n";
            lightOss << std::setprecision(0)
                     << "Shininess: " << material->shininess << " [J/K]\n";
            lightOss << std::setprecision(1)
                     << "Metallic: " << material->metallic << " [N/M]\n"
                     << "Roughness: " << material->roughness << " [U/I]\n";
        }
        lightOss << "DirLight: " << (m_dirLight.enabled ? "ON" : "OFF") << "\n";
        lightOss << "PointLight: " << (m_pointLight.enabled ? "ON" : "OFF") << "\n";
        lightOss << "Scene: save [F11] load [F12]";
        UIText::renderText(lightOss.str(), 10.0f, 290.0f, 1.2f);

        std::ostringstream cubeOss;
        cubeOss << "Objects: " << m_objects.size();
        if (m_selectedObject >= 0 &&
            m_selectedObject < static_cast<int>(m_objects.size())) {
            const RenderObject& object =
                m_objects[static_cast<std::size_t>(m_selectedObject)];
            std::string meshAsset = object.meshAsset;
            if (meshAsset.length() > 42) {
                meshAsset = meshAsset.substr(0, 39) + "...";
            }
            cubeOss << "\nSelected: " << (m_selectedObject + 1) << "/"
                    << m_objects.size() << " " << object.name;
            cubeOss << "\nMesh: " << meshAsset;
            if (object.model != nullptr) {
                cubeOss << "\nGeometry: " << object.model->vertexCount() << " vertices, "
                        << object.model->indexCount() << " indices";
            }
        } else {
            cubeOss << "\nSelected: none";
        }
        cubeOss << "\nSelect: left click / Tab / Shift+Tab"
                << "\nCreate cube: Insert/C | Focus: F"
                << "\nDuplicate: Ctrl+D | Delete: Del"
                << "\nUndo: Ctrl+Z | Redo: Ctrl+Y"
                << "\nHistory: " << m_sceneHistory.undoDepth() << " undo, "
                << m_sceneHistory.redoDepth() << " redo";
        UIText::renderText(cubeOss.str(), 10.0f, 430.0f, 1.2f);
    }
    UIText::flush();
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->onFramebufferResize(width, height);
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onKey(key, action, mods);
    }
}

void Application::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onMouseMove(xpos, ypos);
    }
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->onMouseButton(button, action, mods);
    }
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

void Application::onKey(int key, int action, int mods) {
    if (action != GLFW_PRESS) {
        return;
    }

    if (key == GLFW_KEY_F8 && !m_benchmarkEnabled) {
        Menu::toggle();
        return;
    }
    if (key == GLFW_KEY_F9 && !m_benchmarkEnabled) {
        m_showGPUInfo = !m_showGPUInfo;
        return;
    }
    if (m_benchmarkEnabled && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }
    if (!m_benchmarkEnabled && Menu::isOpen()) {
        int menuKey = key;
        if (key == GLFW_KEY_W || key == GLFW_KEY_KP_8) {
            menuKey = GLFW_KEY_UP;
        } else if (key == GLFW_KEY_S || key == GLFW_KEY_KP_2) {
            menuKey = GLFW_KEY_DOWN;
        } else if (key == GLFW_KEY_KP_ENTER) {
            menuKey = GLFW_KEY_ENTER;
        } else if (key == GLFW_KEY_BACKSPACE) {
            menuKey = GLFW_KEY_ESCAPE;
        }

        if (menuKey == GLFW_KEY_UP || menuKey == GLFW_KEY_DOWN ||
            menuKey == GLFW_KEY_ENTER || menuKey == GLFW_KEY_ESCAPE) {
            Menu::processKey(menuKey);
            return;
        }
    }
    if (!m_benchmarkEnabled && !Menu::isOpen() && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    if (!m_benchmarkEnabled && !Menu::isOpen()) {
        const bool controlPressed = (mods & GLFW_MOD_CONTROL) != 0;
        if (controlPressed && key == GLFW_KEY_Z) {
            restoreSceneHistory((mods & GLFW_MOD_SHIFT) != 0);
            return;
        }
        if (controlPressed && key == GLFW_KEY_Y) {
            restoreSceneHistory(true);
            return;
        }
        if ((key == GLFW_KEY_INSERT || key == GLFW_KEY_C) &&
            (mods & (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER)) == 0) {
            createCubeObject();
            return;
        }
        if (key == GLFW_KEY_F &&
            (mods & (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER)) == 0) {
            focusSelectedObject();
            return;
        }
        if (key == GLFW_KEY_TAB) {
            selectObject((mods & GLFW_MOD_SHIFT) != 0 ? -1 : 1);
            return;
        }
        if (key == GLFW_KEY_D && (mods & GLFW_MOD_CONTROL) != 0) {
            duplicateSelectedObject();
            return;
        }
        if (key == GLFW_KEY_DELETE) {
            deleteSelectedObject();
            return;
        }
        if (key == GLFW_KEY_G) {
            checkpointScene();
            m_showCoordinateGrid = !m_showCoordinateGrid;
            return;
        }
        Material* material = selectedMaterial();
        if (key == GLFW_KEY_L) {
            checkpointScene();
            m_pointLight.enabled = !m_pointLight.enabled;
            return;
        }
        if (key == GLFW_KEY_O) {
            checkpointScene();
            m_dirLight.enabled = !m_dirLight.enabled;
            return;
        }
        if (material != nullptr) {
            if (key == GLFW_KEY_J) {
                const float value = glm::max(2.0f, material->shininess - 8.0f);
                if (value != material->shininess) {
                    checkpointScene();
                    material->shininess = value;
                }
                return;
            }
            if (key == GLFW_KEY_K) {
                const float value = glm::min(256.0f, material->shininess + 8.0f);
                if (value != material->shininess) {
                    checkpointScene();
                    material->shininess = value;
                }
                return;
            }
            if (key == GLFW_KEY_N) {
                const float value = glm::max(0.0f, material->metallic - 0.1f);
                if (value != material->metallic) {
                    checkpointScene();
                    material->metallic = value;
                }
                return;
            }
            if (key == GLFW_KEY_M) {
                const float value = glm::min(1.0f, material->metallic + 0.1f);
                if (value != material->metallic) {
                    checkpointScene();
                    material->metallic = value;
                }
                return;
            }
            if (key == GLFW_KEY_U) {
                const float value = glm::max(0.0f, material->roughness - 0.1f);
                if (value != material->roughness) {
                    checkpointScene();
                    material->roughness = value;
                }
                return;
            }
            if (key == GLFW_KEY_I) {
                const float value = glm::min(1.0f, material->roughness + 0.1f);
                if (value != material->roughness) {
                    checkpointScene();
                    material->roughness = value;
                }
                return;
            }
        }
    }

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_6) {
        const auto effect = static_cast<PostProcessEffect>(key - GLFW_KEY_1);
        if (effect != m_postProcessEffect) {
            checkpointScene();
            m_postProcessEffect = effect;
        }
    } else if (key == GLFW_KEY_F1) {
        if (m_shaderViewMode != 0) {
            checkpointScene();
            m_shaderViewMode = 0;
        }
    } else if (key == GLFW_KEY_F2) {
        if (m_shaderViewMode != 1) {
            checkpointScene();
            m_shaderViewMode = 1;
        }
    } else if (key == GLFW_KEY_F3) {
        if (m_shaderViewMode != 2) {
            checkpointScene();
            m_shaderViewMode = 2;
        }
    } else if (key == GLFW_KEY_F4) {
        if (m_shaderViewMode != 3) {
            checkpointScene();
            m_shaderViewMode = 3;
        }
    } else if (key == GLFW_KEY_F6) {
        if (m_shaderViewMode != 4) {
            checkpointScene();
            m_shaderViewMode = 4;
        }
    } else if (key == GLFW_KEY_F5) {
        m_reloadSucceeded = ResourceManager::reloadAllShaders();
        m_reloadMessage = m_reloadSucceeded ? "Shaders reloaded" : "Shader reload failed";
        m_reloadMessageTime = 2.0f;
    } else if (key == GLFW_KEY_F7) {
        toggleBenchmark();
    } else if (key == GLFW_KEY_F10 && m_benchmarkEnabled) {
        resetFrameStatistics();
        restartBenchmarkCapture();
        std::cout << "GPU benchmark capture restarted\n";
    } else if (key == GLFW_KEY_F11 && !m_benchmarkEnabled) {
        saveQuickScene();
    } else if (key == GLFW_KEY_F12 && !m_benchmarkEnabled) {
        loadQuickScene();
    }
}

void Application::onMouseMove(double xpos, double ypos) {
    if (m_camera == nullptr) return;

    if (m_benchmarkEnabled) {
        m_lastX = static_cast<float>(xpos);
        m_lastY = static_cast<float>(ypos);
        return;
    }
    if (Menu::isOpen()) {
        m_firstMouse = true;
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

void Application::onMouseButton(int button, int action, int mods) {
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
        !m_benchmarkEnabled && !Menu::isOpen()) {
        selectObjectUnderCrosshair();
    }
}

void Application::onScroll(double xoffset, double yoffset) {
    (void)xoffset;
    if (m_camera == nullptr || m_benchmarkEnabled || Menu::isOpen()) return;
    m_camera->processScroll(static_cast<float>(yoffset));
}
