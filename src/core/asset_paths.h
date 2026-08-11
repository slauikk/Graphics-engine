#ifndef ASSET_PATHS_H
#define ASSET_PATHS_H

#include <filesystem>
#include <optional>
#include <string>

namespace core {

    std::filesystem::path executableDir();
    std::filesystem::path findAssetsRoot();
    std::filesystem::path assetPath(const std::string& relativePath);
    std::optional<std::filesystem::path> resolveContainedPath(
        const std::filesystem::path& root,
        const std::filesystem::path& relativeBase,
        const std::filesystem::path& requestedPath);
    std::filesystem::path benchmarkResultsDir();
    std::filesystem::path quickSaveScenePath();
    std::filesystem::path editorSettingsPath();

} // namespace core

#endif // ASSET_PATHS_H
