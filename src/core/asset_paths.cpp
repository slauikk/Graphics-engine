#include "asset_paths.h"
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

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
        static bool initialized = false;
        static std::filesystem::path cachedRoot;

        if (initialized) {
            return cachedRoot;
        }

        std::vector<std::filesystem::path> bases = {
            executableDir(),
            std::filesystem::current_path()
        };

        for (const auto& base : bases) {
            std::vector<std::filesystem::path> candidates = {
                base / "assets",
                base / ".." / "assets",
                base / ".." / ".." / "assets"
            };

            for (const auto& candidate : candidates) {
                std::filesystem::path texturesDir = candidate / "textures";
                std::filesystem::path shadersDir = candidate / "shaders";
                if (std::filesystem::exists(texturesDir) && std::filesystem::is_directory(texturesDir) &&
                    std::filesystem::exists(shadersDir) && std::filesystem::is_directory(shadersDir)) {
                    cachedRoot = candidate;
                    initialized = true;
                    return cachedRoot;
                    }
            }
        }

        cachedRoot = std::filesystem::current_path() / "assets";
        initialized = true;
        return cachedRoot;
    }

    std::filesystem::path assetPath(const std::string& relativePath) {
        return findAssetsRoot() / relativePath;
    }

} // namespace core