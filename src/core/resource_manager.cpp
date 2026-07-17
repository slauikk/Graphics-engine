#include "resource_manager.h"
#include "asset_paths.h"
#include "../shader.h"
#include "../texture2d.h"
#include <filesystem>
#include <unordered_map>

namespace ResourceManager {

namespace {
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaderCache;

    std::shared_ptr<Shader> createFallbackShader() {
        const char* vertexSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 projection;

void main() {
    mat4 model = u_Model;
    mat4 view = u_View;
    mat4 proj = projection;

    if (model[3][3] == 0.0) {
        model = mat4(1.0);
    }
    if (view[3][3] == 0.0) {
        view = mat4(1.0);
    }
    if (proj[3][3] == 0.0) {
        proj = u_Projection;
    }
    if (proj[3][3] == 0.0) {
        proj = mat4(1.0);
    }

    gl_Position = proj * view * model * vec4(aPos, 1.0);
}
)";

        const char* fragmentSource = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)";

        return std::make_shared<Shader>(vertexSource, fragmentSource);
    }
}

std::shared_ptr<Texture2D> getTexture(const std::string& relativePath) {
    auto it = textureCache.find(relativePath);
    if (it != textureCache.end()) {
        return it->second;
    }

    auto texture = std::make_shared<Texture2D>();
    std::filesystem::path fullPath = core::assetPath(relativePath);
    if (std::filesystem::exists(fullPath) && std::filesystem::is_regular_file(fullPath)) {
        texture->loadFromFile(fullPath.string());
    } else {
        texture->loadGeneratedGrid();
    }

    textureCache[relativePath] = texture;
    return texture;
}

std::shared_ptr<Shader> getShader(const std::string& vertRel, const std::string& fragRel) {
    std::string key = vertRel + "|" + fragRel;
    auto it = shaderCache.find(key);
    if (it != shaderCache.end()) {
        return it->second;
    }

    std::filesystem::path vertPath = core::assetPath(vertRel);
    std::filesystem::path fragPath = core::assetPath(fragRel);
    auto shader = std::make_shared<Shader>(vertPath.string(), fragPath.string());

    if (shader->m_id == 0) {
        shader = createFallbackShader();
    }

    shaderCache[key] = shader;
    return shader;
}

bool reloadAllShaders() {
    bool reloadedAny = false;
    bool allSucceeded = true;

    for (auto& entry : shaderCache) {
        if (entry.second) {
            reloadedAny = true;
            allSucceeded = entry.second->reload() && allSucceeded;
        } else {
            allSucceeded = false;
        }
    }

    return reloadedAny && allSucceeded;
}

void clear() {
    shaderCache.clear();
    textureCache.clear();
}

} // namespace ResourceManager
