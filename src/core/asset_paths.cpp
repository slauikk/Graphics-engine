#include "asset_paths.h"
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

bool isAssetsRoot(const std::filesystem::path& candidate) {
    std::error_code error;
    const bool hasTextures = std::filesystem::is_directory(candidate / "textures", error);
    if (error) {
        return false;
    }

    const bool hasShaders = std::filesystem::is_directory(candidate / "shaders", error);
    return !error && hasTextures && hasShaders;
}

std::filesystem::path normalizedPath(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, error);
    return error ? path.lexically_normal() : normalized;
}

} // namespace

namespace core {

    std::filesystem::path executableDir() {
#ifdef _WIN32
        std::wstring buffer(MAX_PATH, L'\0');
        DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) {
            return std::filesystem::current_path();
        }
        buffer.resize(size);
        return std::filesystem::path(buffer).parent_path();
#else
        return std::filesystem::current_path();
#endif
    }

    std::filesystem::path findAssetsRoot() {
        static const std::filesystem::path cachedRoot = [] {
#ifdef GRAPHICS_ENGINE_SOURCE_ASSETS_DIR
            const std::filesystem::path sourceAssets = GRAPHICS_ENGINE_SOURCE_ASSETS_DIR;
            if (isAssetsRoot(sourceAssets)) {
                return normalizedPath(sourceAssets);
            }
#endif

            const std::vector<std::filesystem::path> bases = {
                executableDir(),
                std::filesystem::current_path()
            };

            for (const auto& base : bases) {
                const std::vector<std::filesystem::path> candidates = {
                    base / "assets",
                    base / ".." / "assets",
                    base / ".." / ".." / "assets"
                };

                for (const auto& candidate : candidates) {
                    if (isAssetsRoot(candidate)) {
                        return normalizedPath(candidate);
                    }
                }
            }

            return normalizedPath(std::filesystem::current_path() / "assets");
        }();

        return cachedRoot;
    }

    std::filesystem::path assetPath(const std::string& relativePath) {
        return findAssetsRoot() / relativePath;
    }

} // namespace core
