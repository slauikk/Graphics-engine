#include "asset_paths.h"
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace {

bool isAssetsRoot(const std::filesystem::path& candidate) {
    std::error_code error;
    const bool hasTextures = std::filesystem::is_directory(candidate / "textures", error);
    if (error) {
        return false;
    }

    const bool hasShaders = std::filesystem::is_directory(candidate / "shaders", error);
    if (error || !hasTextures || !hasShaders) {
        return false;
    }

    const std::filesystem::path shaders = candidate / "shaders";
    const std::filesystem::path requiredShaders[] = {
        shaders / "textured.vert",
        shaders / "textured.frag",
        shaders / "post_process.vert",
        shaders / "post_process.frag",
        shaders / "selection_outline.vert",
        shaders / "selection_outline.frag",
        shaders / "ui_text.vert",
        shaders / "ui_text.frag",
        shaders / "ui_rect.vert",
        shaders / "ui_rect.frag"
    };
    for (const auto& shader : requiredShaders) {
        error.clear();
        if (!std::filesystem::is_regular_file(shader, error) || error) {
            return false;
        }
    }
    return true;
}

std::filesystem::path normalizedPath(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::path normalized = std::filesystem::weakly_canonical(path, error);
    return error ? path.lexically_normal() : normalized;
}

std::filesystem::path safeCurrentPath() {
    std::error_code error;
    std::filesystem::path current = std::filesystem::current_path(error);
    if (!error && !current.empty()) {
        return current;
    }

    error.clear();
    std::filesystem::path temporary = std::filesystem::temp_directory_path(error);
    return !error && !temporary.empty() ? temporary : std::filesystem::path(".");
}

std::filesystem::path temporaryResultsDir() {
    std::error_code error;
    std::filesystem::path temporary = std::filesystem::temp_directory_path(error);
    if (error || temporary.empty()) {
        temporary = safeCurrentPath();
    }
    return normalizedPath(temporary / "Graphics_engine" / "benchmark-results");
}

#ifdef _WIN32
std::filesystem::path windowsEnvironmentPath(const wchar_t* variable) {
    DWORD capacity = GetEnvironmentVariableW(variable, nullptr, 0);
    if (capacity == 0) {
        return {};
    }

    std::vector<wchar_t> buffer(capacity);
    while (true) {
        const DWORD size = GetEnvironmentVariableW(
            variable, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) {
            return {};
        }
        if (size < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), size));
        }
        if (size == (std::numeric_limits<DWORD>::max)()) {
            return {};
        }
        buffer.resize(static_cast<std::size_t>(size) + 1);
    }
}
#else
std::filesystem::path environmentPath(const char* variable) {
    const char* value = std::getenv(variable);
    return value != nullptr && value[0] != '\0' ? std::filesystem::path(value)
                                                  : std::filesystem::path();
}
#endif

} // namespace

namespace core {

    std::filesystem::path executableDir() {
#ifdef _WIN32
        std::vector<wchar_t> buffer(512);
        while (buffer.size() <= (std::numeric_limits<DWORD>::max)()) {
            const DWORD size = GetModuleFileNameW(
                nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (size == 0) {
                break;
            }
            if (size < buffer.size()) {
                return normalizedPath(
                    std::filesystem::path(std::wstring(buffer.data(), size))).parent_path();
            }
            if (buffer.size() > (std::numeric_limits<DWORD>::max)() / 2) {
                break;
            }
            buffer.resize(buffer.size() * 2);
        }

#elif defined(__APPLE__)
        uint32_t capacity = 1024;
        std::vector<char> buffer(capacity);
        while (_NSGetExecutablePath(buffer.data(), &capacity) != 0) {
            if (capacity <= buffer.size()) {
                break;
            }
            buffer.resize(capacity);
        }
        if (capacity <= buffer.size() && !buffer.empty() && buffer.front() != '\0') {
            return normalizedPath(std::filesystem::path(buffer.data())).parent_path();
        }

#elif defined(__linux__)
        std::vector<char> buffer(512);
        while (true) {
            const ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size());
            if (size < 0) {
                break;
            }
            if (static_cast<std::size_t>(size) < buffer.size()) {
                return normalizedPath(std::filesystem::path(
                    std::string(buffer.data(), static_cast<std::size_t>(size)))).parent_path();
            }
            if (buffer.size() > std::numeric_limits<std::size_t>::max() / 2) {
                break;
            }
            buffer.resize(buffer.size() * 2);
        }
#else
        return safeCurrentPath();
#endif

        return safeCurrentPath();
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
                safeCurrentPath()
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

            return normalizedPath(safeCurrentPath() / "assets");
        }();

        return cachedRoot;
    }

    std::filesystem::path assetPath(const std::string& relativePath) {
        return findAssetsRoot() / relativePath;
    }

    std::optional<std::filesystem::path> resolveContainedPath(
        const std::filesystem::path& root,
        const std::filesystem::path& relativeBase,
        const std::filesystem::path& requestedPath) {
        if (root.empty() || relativeBase.empty() || requestedPath.empty()) {
            return std::nullopt;
        }
        if (!requestedPath.is_absolute() &&
            (requestedPath.has_root_name() || requestedPath.has_root_directory())) {
            return std::nullopt;
        }

        std::error_code error;
        const std::filesystem::path resolvedRoot =
            std::filesystem::weakly_canonical(root, error);
        if (error || resolvedRoot.empty() || !resolvedRoot.is_absolute()) {
            return std::nullopt;
        }

        const std::filesystem::path candidate = requestedPath.is_absolute()
            ? requestedPath
            : relativeBase / requestedPath;
        error.clear();
        const std::filesystem::path resolvedCandidate =
            std::filesystem::weakly_canonical(candidate, error);
        if (error || resolvedCandidate.empty() || !resolvedCandidate.is_absolute()) {
            return std::nullopt;
        }

        error.clear();
        const std::filesystem::path relative =
            std::filesystem::relative(resolvedCandidate, resolvedRoot, error);
        if (error || relative.empty() || relative.is_absolute()) {
            return std::nullopt;
        }
        for (const auto& component : relative) {
            if (component == "..") {
                return std::nullopt;
            }
        }
        return resolvedCandidate;
    }

    std::filesystem::path benchmarkResultsDir() {
#ifdef GRAPHICS_ENGINE_SOURCE_ASSETS_DIR
        return normalizedPath(std::filesystem::path(GRAPHICS_ENGINE_SOURCE_ASSETS_DIR))
            .parent_path() / "benchmark-results";
#elif defined(_WIN32)
        const std::filesystem::path localAppData = windowsEnvironmentPath(L"LOCALAPPDATA");
        return localAppData.empty()
            ? temporaryResultsDir()
            : normalizedPath(localAppData / "Graphics_engine" / "benchmark-results");
#elif defined(__APPLE__)
        const std::filesystem::path home = environmentPath("HOME");
        return home.empty()
            ? temporaryResultsDir()
            : normalizedPath(home / "Library" / "Application Support" /
                             "Graphics_engine" / "benchmark-results");
#elif defined(__linux__)
        const std::filesystem::path stateHome = environmentPath("XDG_STATE_HOME");
        if (!stateHome.empty() && stateHome.is_absolute()) {
            return normalizedPath(stateHome / "Graphics_engine" / "benchmark-results");
        }

        const std::filesystem::path home = environmentPath("HOME");
        return home.empty()
            ? temporaryResultsDir()
            : normalizedPath(home / ".local" / "state" /
                             "Graphics_engine" / "benchmark-results");
#else
        return temporaryResultsDir();
#endif
    }

    std::filesystem::path quickSaveScenePath() {
        return benchmarkResultsDir().parent_path() /
               "saved-scenes" / "quick_save.scene.json";
    }

    std::filesystem::path editorSettingsPath() {
#ifdef GRAPHICS_ENGINE_SOURCE_ASSETS_DIR
        return normalizedPath(std::filesystem::path(GRAPHICS_ENGINE_SOURCE_ASSETS_DIR))
            .parent_path() / "out" / "runtime" / "editor_settings.json";
#else
        return benchmarkResultsDir().parent_path() / "editor_settings.json";
#endif
    }

} // namespace core
