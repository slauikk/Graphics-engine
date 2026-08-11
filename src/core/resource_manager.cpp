#include "resource_manager.h"
#include "asset_paths.h"
#include "../shader.h"
#include "../texture2d.h"

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

namespace ResourceManager {

namespace {

struct ShaderCacheKey {
    std::string vertexPath;
    std::string fragmentPath;

    bool operator==(const ShaderCacheKey&) const = default;
};

struct ShaderCacheKeyHash {
    std::size_t operator()(const ShaderCacheKey& key) const noexcept {
        const std::size_t vertexHash = std::hash<std::string>{}(key.vertexPath);
        const std::size_t fragmentHash = std::hash<std::string>{}(key.fragmentPath);
        return vertexHash ^ (fragmentHash + 0x9e3779b9U +
                             (vertexHash << 6U) + (vertexHash >> 2U));
    }
};

std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache;
std::unordered_map<ShaderCacheKey, std::shared_ptr<Shader>, ShaderCacheKeyHash> shaderCache;

enum class ShaderKind {
    Mesh,
    UiText,
    UiRect,
    PostProcess
};

constexpr const char* kMeshFallbackVertex = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in mat4 aInstanceModel;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform int u_UseInstancing;

void main() {
    mat4 model = u_UseInstancing != 0 ? aInstanceModel : u_Model;
    gl_Position = u_Projection * u_View * model * vec4(aPos, 1.0);
}
)";

constexpr const char* kMeshFallbackFragment = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
)";

constexpr const char* kUiFallbackVertex = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aColor;

out vec2 TexCoord;
out vec3 TextColor;

uniform mat4 projection;

void main() {
    TexCoord = aTexCoord;
    TextColor = aColor;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)";

constexpr const char* kUiFallbackFragment = R"(
#version 330 core
in vec2 TexCoord;
in vec3 TextColor;

out vec4 FragColor;

uniform sampler2D fontAtlas;

void main() {
    float coverage = texture(fontAtlas, TexCoord).r;
    if (coverage < 0.5) {
        discard;
    }
    FragColor = vec4(TextColor, coverage);
}
)";

constexpr const char* kUiRectFallbackVertex = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

out vec3 RectColor;
uniform mat4 projection;

void main() {
    RectColor = aColor;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)";

constexpr const char* kUiRectFallbackFragment = R"(
#version 330 core
in vec3 RectColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(RectColor, 1.0);
}
)";

constexpr const char* kPostProcessFallbackVertex = R"(
#version 330 core
out vec2 TexCoord;

void main() {
    const vec2 positions[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 position = positions[gl_VertexID];
    TexCoord = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* kPostProcessFallbackFragment = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D u_Scene;
uniform int u_Effect;
uniform float u_Time;

void main() {
    vec3 color = texture(u_Scene, TexCoord).rgb;
    if (u_Effect < 0 && u_Time < 0.0) {
        color = vec3(1.0) - color;
    }
    FragColor = vec4(color, 1.0);
}
)";

ShaderKind shaderKind(const ShaderCacheKey& key) {
    const auto vertexFilename = std::filesystem::path(key.vertexPath).filename();
    const auto fragmentFilename = std::filesystem::path(key.fragmentPath).filename();
    if (vertexFilename == "ui_text.vert" && fragmentFilename == "ui_text.frag") {
        return ShaderKind::UiText;
    }
    if (vertexFilename == "ui_rect.vert" && fragmentFilename == "ui_rect.frag") {
        return ShaderKind::UiRect;
    }
    if (vertexFilename == "post_process.vert" && fragmentFilename == "post_process.frag") {
        return ShaderKind::PostProcess;
    }
    return ShaderKind::Mesh;
}

bool hasUiTextInterface(const Shader& shader) {
    if (shader.m_id == 0) {
        return false;
    }

    return glGetAttribLocation(shader.m_id, "aPos") == 0 &&
           glGetAttribLocation(shader.m_id, "aTexCoord") == 1 &&
           glGetAttribLocation(shader.m_id, "aColor") == 2 &&
           glGetUniformLocation(shader.m_id, "projection") >= 0 &&
           glGetUniformLocation(shader.m_id, "fontAtlas") >= 0;
}

bool hasPostProcessInterface(const Shader& shader) {
    return shader.m_id != 0 &&
           glGetUniformLocation(shader.m_id, "u_Scene") >= 0 &&
           glGetUniformLocation(shader.m_id, "u_Effect") >= 0 &&
           glGetUniformLocation(shader.m_id, "u_Time") >= 0;
}

bool hasUiRectInterface(const Shader& shader) {
    return shader.m_id != 0 &&
           glGetAttribLocation(shader.m_id, "aPos") == 0 &&
           glGetAttribLocation(shader.m_id, "aColor") == 1 &&
           glGetUniformLocation(shader.m_id, "projection") >= 0;
}

bool hasExpectedInterface(const Shader& shader, ShaderKind kind) {
    switch (kind) {
        case ShaderKind::UiText: return hasUiTextInterface(shader);
        case ShaderKind::UiRect: return hasUiRectInterface(shader);
        case ShaderKind::PostProcess: return hasPostProcessInterface(shader);
        case ShaderKind::Mesh:
        default: return shader.m_id != 0;
    }
}

bool loadFallback(Shader& shader, ShaderKind kind) {
    switch (kind) {
        case ShaderKind::UiText:
            return shader.loadFromSource(kUiFallbackVertex, kUiFallbackFragment);
        case ShaderKind::UiRect:
            return shader.loadFromSource(
                kUiRectFallbackVertex, kUiRectFallbackFragment);
        case ShaderKind::PostProcess:
            return shader.loadFromSource(
                kPostProcessFallbackVertex, kPostProcessFallbackFragment);
        case ShaderKind::Mesh:
        default:
            return shader.loadFromSource(kMeshFallbackVertex, kMeshFallbackFragment);
    }
}

} // namespace

std::shared_ptr<Texture2D> getTexture(const std::string& relativePath) {
    auto it = textureCache.find(relativePath);
    if (it != textureCache.end()) {
        return it->second;
    }

    auto texture = std::make_shared<Texture2D>();
    const std::filesystem::path fullPath = core::assetPath(relativePath);
    std::error_code filesystemError;
    if (std::filesystem::is_regular_file(fullPath, filesystemError) && !filesystemError) {
        texture->loadFromFile(fullPath);
    } else {
        texture->loadGeneratedGrid();
    }

    textureCache[relativePath] = texture;
    return texture;
}

std::shared_ptr<Shader> getShader(const std::string& vertRel, const std::string& fragRel) {
    const ShaderCacheKey key{vertRel, fragRel};
    auto it = shaderCache.find(key);
    if (it != shaderCache.end()) {
        return it->second;
    }

    const std::filesystem::path vertPath = core::assetPath(vertRel);
    const std::filesystem::path fragPath = core::assetPath(fragRel);
    auto shader = std::make_shared<Shader>(vertPath, fragPath);

    const ShaderKind kind = shaderKind(key);
    const bool needsFallback = !hasExpectedInterface(*shader, kind);
    if (needsFallback) {
        if (shader->m_id != 0) {
            std::cerr << "[ResourceManager] Shader interface is incompatible; using fallback\n";
        }
        if (!loadFallback(*shader, kind)) {
            std::cerr << "[ResourceManager] Failed to build fallback shader\n";
        }
    }

    shaderCache[key] = shader;
    return shader;
}

bool reloadAllShaders() {
    bool reloadedAny = false;
    bool allSucceeded = true;

    for (auto& [key, shader] : shaderCache) {
        if (!shader) {
            allSucceeded = false;
            continue;
        }

        reloadedAny = true;
        bool succeeded = shader->reload();
        const ShaderKind kind = shaderKind(key);
        if (succeeded && !hasExpectedInterface(*shader, kind)) {
            std::cerr << "[ResourceManager] Reloaded shader has an incompatible interface\n";
            loadFallback(*shader, kind);
            succeeded = false;
        }
        allSucceeded = succeeded && allSucceeded;
    }

    return reloadedAny && allSucceeded;
}

void clear() {
    shaderCache.clear();
    textureCache.clear();
}

} // namespace ResourceManager
