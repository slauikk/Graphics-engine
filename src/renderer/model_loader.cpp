#include "model_loader.h"

#include "model.h"
#include "mesh/mesh_factory.h"
#include "../core/asset_paths.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>
#include <unordered_set>

#include <assimp/DefaultIOSystem.h>
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace {

constexpr std::uintmax_t kMaxModelFileSize = 512U * 1024U * 1024U;
constexpr std::size_t kMaxMeshes = 4'096;
constexpr std::size_t kMaxMaterials = 4'096;
constexpr std::size_t kMaxVertices = 10'000'000;
constexpr std::size_t kMaxIndices = 30'000'000;
constexpr std::size_t kMaxAssetReferenceLength = 1'024;
constexpr float kMaxCoordinate = 1'000'000.0f;
constexpr float kMaxColorComponent = 100.0f;

std::string normalizedFileKey(const std::filesystem::path& filePath) {
    std::error_code error;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(
        filePath, error);
    if (error) {
        normalized = filePath.lexically_normal();
    }
    std::string key = normalized.generic_string();
#ifdef _WIN32
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
#endif
    return key;
}

class LimitedIOSystem final : public Assimp::IOSystem {
public:
    LimitedIOSystem(std::filesystem::path assetsRoot,
                    std::filesystem::path modelDirectory)
        : m_assetsRoot(std::move(assetsRoot)),
          m_modelDirectory(std::move(modelDirectory)) {
    }

    bool Exists(const char* filePath) const override {
        const auto resolved = resolve(filePath);
        if (!resolved) {
            return false;
        }
        const std::string nativePath = resolved->string();
        return m_delegate.Exists(nativePath.c_str());
    }

    char getOsSeparator() const override {
        return m_delegate.getOsSeparator();
    }

    Assimp::IOStream* Open(const char* filePath, const char* mode) override {
        const char* accessMode = mode != nullptr ? mode : "rb";
        const std::string_view access(accessMode);
        if (access != "r" && access != "rb" && access != "rt") {
            m_policyViolated = true;
            return nullptr;
        }

        const auto resolved = resolve(filePath);
        if (!resolved) {
            return nullptr;
        }
        const std::string nativePath = resolved->string();
        Assimp::IOStream* stream = m_delegate.Open(nativePath.c_str(), accessMode);
        if (stream == nullptr) {
            return nullptr;
        }

        const std::string key = normalizedFileKey(*resolved);
        if (!m_countedFiles.contains(key)) {
            const std::size_t fileSize = stream->FileSize();
            const std::size_t byteLimit = static_cast<std::size_t>(kMaxModelFileSize);
            if (fileSize > byteLimit || fileSize > byteLimit - m_totalBytes) {
                m_limitExceeded = true;
                m_delegate.Close(stream);
                return nullptr;
            }
            m_countedFiles.insert(key);
            m_totalBytes += fileSize;
        }
        return stream;
    }

    void Close(Assimp::IOStream* stream) override {
        m_delegate.Close(stream);
    }

    bool ComparePaths(const char* left, const char* right) const override {
        return m_delegate.ComparePaths(left, right);
    }

    bool limitExceeded() const {
        return m_limitExceeded;
    }

    bool policyViolated() const {
        return m_policyViolated;
    }

private:
    std::optional<std::filesystem::path> resolve(const char* filePath) const {
        if (filePath == nullptr || filePath[0] == '\0') {
            m_policyViolated = true;
            return std::nullopt;
        }
        auto resolved = core::resolveContainedPath(
            m_assetsRoot, m_modelDirectory, std::filesystem::path(filePath));
        if (!resolved) {
            m_policyViolated = true;
        }
        return resolved;
    }

    Assimp::DefaultIOSystem m_delegate;
    std::filesystem::path m_assetsRoot;
    std::filesystem::path m_modelDirectory;
    std::unordered_set<std::string> m_countedFiles;
    std::size_t m_totalBytes = 0;
    bool m_limitExceeded = false;
    mutable bool m_policyViolated = false;
};

bool isSafeAssetReference(std::string_view value) {
    if (value.empty() || value.size() > kMaxAssetReferenceLength ||
        value.find('\\') != std::string_view::npos ||
        value.find(':') != std::string_view::npos) {
        return false;
    }
    for (const unsigned char character : value) {
        if (character < 0x20U || character == 0x7fU) {
            return false;
        }
    }

    const std::filesystem::path path(value);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return false;
    }
    for (const auto& component : path) {
        if (component == "." || component == "..") {
            return false;
        }
    }
    return true;
}

bool isFinite(float value) {
    return std::isfinite(value);
}

bool isValidPosition(const aiVector3D& value) {
    return isFinite(value.x) && isFinite(value.y) && isFinite(value.z) &&
           std::abs(value.x) <= kMaxCoordinate &&
           std::abs(value.y) <= kMaxCoordinate &&
           std::abs(value.z) <= kMaxCoordinate;
}

bool normalizeOrReplaceNormal(aiVector3D& value) {
    if (!isFinite(value.x) || !isFinite(value.y) || !isFinite(value.z) ||
        std::abs(value.x) > kMaxCoordinate ||
        std::abs(value.y) > kMaxCoordinate ||
        std::abs(value.z) > kMaxCoordinate) {
        value = aiVector3D(0.0f, 1.0f, 0.0f);
        return true;
    }

    const float lengthSquared = value.x * value.x + value.y * value.y + value.z * value.z;
    if (!isFinite(lengthSquared) || lengthSquared < 0.000001f) {
        value = aiVector3D(0.0f, 1.0f, 0.0f);
        return true;
    }
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    value.x *= inverseLength;
    value.y *= inverseLength;
    value.z *= inverseLength;
    return false;
}

glm::vec3 sanitizedColor(const aiColor4D& color, const glm::vec3& fallback,
                         std::string_view label, std::vector<std::string>& warnings) {
    const std::array<float, 3> imported = {color.r, color.g, color.b};
    const std::array<float, 3> defaults = {fallback.x, fallback.y, fallback.z};
    std::array<float, 3> result = defaults;
    bool sanitized = false;
    for (std::size_t index = 0; index < imported.size(); ++index) {
        if (isFinite(imported[index])) {
            result[index] = std::clamp(imported[index], 0.0f, kMaxColorComponent);
            sanitized = sanitized || result[index] != imported[index];
        } else {
            sanitized = true;
        }
    }
    if (sanitized) {
        warnings.push_back(std::string(label) + " color contained invalid values and was clamped");
    }
    return glm::vec3(result[0], result[1], result[2]);
}

float sanitizedFactor(float value, float fallback, float minimum, float maximum,
                      std::string_view label, std::vector<std::string>& warnings) {
    if (!isFinite(value)) {
        warnings.push_back(std::string(label) + " contained a non-finite value; using default");
        return fallback;
    }
    return std::clamp(value, minimum, maximum);
}

std::string lowercaseExtension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return extension;
}

std::uint64_t fnv1a(std::string_view value) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char character : value) {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string materialId(const std::string& assetReference, std::size_t index) {
    std::ostringstream output;
    output << "model:" << std::hex << std::setfill('0') << std::setw(16)
           << fnv1a(assetReference) << ":" << std::dec << index;
    return output.str();
}

std::string safeTextureReference(const aiString& importedPath,
                                 const std::filesystem::path& modelPath,
                                 std::vector<std::string>& warnings) {
    std::string pathText = importedPath.C_Str();
    if (pathText.empty()) {
        return {};
    }
    if (pathText.front() == '*') {
        warnings.push_back("embedded material texture is not yet decoded");
        return {};
    }
    std::replace(pathText.begin(), pathText.end(), '\\', '/');

    std::error_code error;
    const std::filesystem::path assetsRoot =
        std::filesystem::weakly_canonical(core::findAssetsRoot(), error);
    if (error) {
        warnings.push_back("failed to resolve assets root for material texture");
        return {};
    }
    const std::filesystem::path texturePath = std::filesystem::weakly_canonical(
        modelPath.parent_path() / std::filesystem::path(pathText), error);
    if (error) {
        warnings.push_back("failed to resolve material texture: " + pathText);
        return {};
    }
    const std::filesystem::path relative =
        std::filesystem::relative(texturePath, assetsRoot, error);
    if (error || relative.empty() || relative.is_absolute()) {
        warnings.push_back("material texture is outside the assets directory: " + pathText);
        return {};
    }
    for (const auto& component : relative) {
        if (component == "..") {
            warnings.push_back("material texture is outside the assets directory: " + pathText);
            return {};
        }
    }
    return relative.generic_string();
}

ImportedMaterial readMaterial(const aiMaterial& source, std::size_t index,
                              const std::string& assetReference,
                              const std::filesystem::path& modelPath,
                              std::vector<std::string>& warnings) {
    ImportedMaterial material;
    material.id = materialId(assetReference, index);

    aiString name;
    if (source.Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        material.name = name.C_Str();
    }

    aiColor4D color;
    if (aiGetMaterialColor(&source, AI_MATKEY_BASE_COLOR, &color) == AI_SUCCESS ||
        aiGetMaterialColor(&source, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
        material.albedoColor = sanitizedColor(
            color, material.albedoColor, "albedo", warnings);
    }
    if (aiGetMaterialColor(&source, AI_MATKEY_COLOR_SPECULAR, &color) == AI_SUCCESS) {
        material.specularColor = sanitizedColor(
            color, material.specularColor, "specular", warnings);
    }
    if (aiGetMaterialColor(&source, AI_MATKEY_COLOR_EMISSIVE, &color) == AI_SUCCESS) {
        material.emissiveColor = sanitizedColor(
            color, material.emissiveColor, "emissive", warnings);
    }

    float value = 0.0f;
    if (aiGetMaterialFloat(&source, AI_MATKEY_SHININESS, &value) == AI_SUCCESS) {
        material.shininess = sanitizedFactor(
            value, material.shininess, 1.0f, 1'024.0f, "shininess", warnings);
    }
    if (aiGetMaterialFloat(&source, AI_MATKEY_METALLIC_FACTOR, &value) == AI_SUCCESS) {
        material.metallic = sanitizedFactor(
            value, material.metallic, 0.0f, 1.0f, "metallic factor", warnings);
    }
    if (aiGetMaterialFloat(&source, AI_MATKEY_ROUGHNESS_FACTOR, &value) == AI_SUCCESS) {
        material.roughness = sanitizedFactor(
            value, material.roughness, 0.0f, 1.0f, "roughness factor", warnings);
    }

    aiString texturePath;
    if (source.GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) != AI_SUCCESS) {
        source.GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
    }
    material.albedoTexture = safeTextureReference(
        texturePath, modelPath, warnings);
    return material;
}

} // namespace

namespace ModelLoader {

ModelLoadResult load(const std::string& assetReference) {
    ModelLoadResult result;
    if (!isSafeAssetReference(assetReference)) {
        result.error = "model asset reference is unsafe";
        return result;
    }

    const std::filesystem::path modelPath = core::assetPath(assetReference);
    const std::string extension = lowercaseExtension(modelPath);
    if (extension != ".obj" && extension != ".gltf" && extension != ".glb") {
        result.error = "unsupported model format: " + extension;
        return result;
    }

    std::error_code filesystemError;
    const std::uintmax_t fileSize = std::filesystem::file_size(modelPath, filesystemError);
    if (filesystemError) {
        result.error = "model file is missing or unreadable";
        return result;
    }
    if (fileSize > kMaxModelFileSize) {
        result.error = "model file exceeds the 512 MiB limit";
        return result;
    }

    const std::filesystem::path assetsRoot = core::findAssetsRoot();
    Assimp::Importer importer;
    auto* limitedIo = new LimitedIOSystem(assetsRoot, modelPath.parent_path());
    importer.SetIOHandler(limitedIo);
    importer.SetPropertyInteger(
        AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    constexpr unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_ImproveCacheLocality |
        aiProcess_FindInvalidData |
        aiProcess_SortByPType |
        aiProcess_PreTransformVertices |
        aiProcess_ValidateDataStructure;
    const aiScene* scene = importer.ReadFile(modelPath.string(), flags);
    if (limitedIo->policyViolated()) {
        result.error = "model or dependency attempted file access outside the assets directory";
        return result;
    }
    if (scene == nullptr || scene->mRootNode == nullptr) {
        result.error = limitedIo->limitExceeded()
            ? "model dependencies exceed the 512 MiB total limit"
            : std::string("Assimp failed to import model: ") + importer.GetErrorString();
        return result;
    }
    if (scene->mNumMeshes == 0 || scene->mNumMeshes > kMaxMeshes) {
        result.error = "model has an unsupported number of meshes";
        return result;
    }
    if (scene->mNumMaterials > kMaxMaterials) {
        result.error = "model has an unsupported number of materials";
        return result;
    }

    result.materials.reserve(scene->mNumMaterials);
    for (unsigned int index = 0; index < scene->mNumMaterials; ++index) {
        if (scene->mMaterials[index] != nullptr) {
            result.materials.push_back(readMaterial(
                *scene->mMaterials[index], index, assetReference, modelPath, result.warnings));
        } else {
            ImportedMaterial fallback;
            fallback.id = materialId(assetReference, index);
            result.materials.push_back(std::move(fallback));
        }
    }
    if (result.materials.empty()) {
        ImportedMaterial fallback;
        fallback.id = materialId(assetReference, 0);
        result.materials.push_back(std::move(fallback));
    }

    const bool flipTextureV = extension == ".gltf" || extension == ".glb";
    std::vector<ModelPart> parts;
    parts.reserve(scene->mNumMeshes);
    std::size_t totalVertices = 0;
    std::size_t totalIndices = 0;
    bool warnedAboutNormal = false;
    bool warnedAboutTextureCoordinate = false;
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* source = scene->mMeshes[meshIndex];
        if (source == nullptr || source->mNumVertices == 0 || source->mNumFaces == 0 ||
            (source->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) {
            continue;
        }
        if (source->mNumVertices > kMaxVertices - totalVertices) {
            result.error = "model exceeds the vertex limit";
            return result;
        }

        std::vector<float> vertices;
        vertices.reserve(static_cast<std::size_t>(source->mNumVertices) * 8U);
        for (unsigned int vertexIndex = 0; vertexIndex < source->mNumVertices; ++vertexIndex) {
            const aiVector3D& position = source->mVertices[vertexIndex];
            if (!isValidPosition(position)) {
                result.error = "model contains a non-finite or out-of-range position";
                return result;
            }

            aiVector3D normal = source->HasNormals()
                ? source->mNormals[vertexIndex]
                : aiVector3D(0.0f, 1.0f, 0.0f);
            if (normalizeOrReplaceNormal(normal) && !warnedAboutNormal) {
                result.warnings.push_back("invalid vertex normal replaced with an up vector");
                warnedAboutNormal = true;
            }
            aiVector3D uv(0.0f);
            if (source->HasTextureCoords(0)) {
                uv = source->mTextureCoords[0][vertexIndex];
                if (!isFinite(uv.x) || !isFinite(uv.y) ||
                    std::abs(uv.x) > kMaxCoordinate ||
                    std::abs(uv.y) > kMaxCoordinate) {
                    uv = aiVector3D(0.0f);
                    if (!warnedAboutTextureCoordinate) {
                        result.warnings.push_back(
                            "invalid texture coordinate replaced with zero");
                        warnedAboutTextureCoordinate = true;
                    }
                }
                if (flipTextureV) {
                    uv.y = 1.0f - uv.y;
                }
            }
            const std::array<float, 8> vertex = {
                position.x, position.y, position.z,
                normal.x, normal.y, normal.z,
                uv.x, uv.y
            };
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());
        }

        if (source->mNumFaces > (kMaxIndices - totalIndices) / 3U) {
            result.error = "model exceeds the index limit";
            return result;
        }

        std::vector<std::uint32_t> indices;
        indices.reserve(static_cast<std::size_t>(source->mNumFaces) * 3U);
        for (unsigned int faceIndex = 0; faceIndex < source->mNumFaces; ++faceIndex) {
            const aiFace& face = source->mFaces[faceIndex];
            if (face.mNumIndices != 3 || face.mIndices == nullptr) {
                result.error = "triangulated model contains a non-triangle face";
                return result;
            }
            for (unsigned int index = 0; index < 3; ++index) {
                if (face.mIndices[index] >= source->mNumVertices) {
                    result.error = "model contains an out-of-range vertex index";
                    return result;
                }
                indices.push_back(face.mIndices[index]);
            }
        }
        if (indices.size() > kMaxIndices - totalIndices) {
            result.error = "model exceeds the index limit";
            return result;
        }

        std::shared_ptr<Mesh> mesh = MeshFactory::CreateInterleaved(vertices, indices);
        if (!mesh) {
            result.error = "failed to upload imported mesh to the GPU";
            return result;
        }
        const std::size_t materialIndex = source->mMaterialIndex < result.materials.size()
            ? source->mMaterialIndex
            : 0U;
        parts.push_back({std::move(mesh), result.materials[materialIndex].id});
        totalVertices += source->mNumVertices;
        totalIndices += indices.size();
    }

    if (parts.empty()) {
        result.error = "model contains no triangle meshes";
        return result;
    }
    result.model = std::make_shared<Model>(
        std::move(parts), totalVertices, totalIndices);
    return result;
}

} // namespace ModelLoader
