#ifndef ASSET_PATHS_H
#define ASSET_PATHS_H

#include <filesystem>
#include <string>

namespace core {

    std::filesystem::path executableDir();
    std::filesystem::path findAssetsRoot();
    std::filesystem::path assetPath(const std::string& relativePath);
    std::filesystem::path benchmarkResultsDir();
    std::filesystem::path quickSaveScenePath();

} // namespace core

#endif // ASSET_PATHS_H
