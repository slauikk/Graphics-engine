#include "window_settings.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

using Json = nlohmann::json;
constexpr std::uintmax_t kMaximumWindowSettingsFileSize = 64U * 1024U;
constexpr int kMaximumStoredWindowPosition = 1'000'000;

bool setError(std::string& error, const char* message) {
    error = message;
    return false;
}

bool readInteger(const Json& object, const char* key, int& destination) {
    const auto value = object.find(key);
    if (value == object.end() || !value->is_number()) {
        return false;
    }
    const double number = value->get<double>();
    if (!std::isfinite(number) || std::trunc(number) != number ||
        number < static_cast<double>((std::numeric_limits<int>::min)()) ||
        number > static_cast<double>((std::numeric_limits<int>::max)())) {
        return false;
    }
    destination = static_cast<int>(number);
    return true;
}

bool readBoolean(const Json& object, const char* key, bool& destination) {
    const auto value = object.find(key);
    if (value == object.end() || !value->is_boolean()) {
        return false;
    }
    destination = value->get<bool>();
    return true;
}

bool replaceFile(
    const std::filesystem::path& temporary,
    const std::filesystem::path& destination,
    std::string& error) {
#ifdef _WIN32
    if (MoveFileExW(
            temporary.c_str(), destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE) {
        return true;
    }
    error = "failed to replace window settings: " +
        std::error_code(
            static_cast<int>(GetLastError()), std::system_category())
            .message();
#else
    std::error_code filesystemError;
    std::filesystem::rename(temporary, destination, filesystemError);
    if (!filesystemError) {
        return true;
    }
    error = "failed to replace window settings: " + filesystemError.message();
#endif
    return false;
}

} // namespace

namespace core {

bool validateWindowSettings(
    const WindowSettings& settings,
    std::string& error) {
    if (settings.schemaVersion != kCurrentWindowSettingsSchemaVersion) {
        return setError(error, "unsupported window settings schema version");
    }
    if (settings.width <= 0 ||
        settings.height <= 0 ||
        settings.width > kMaximumEditorWindowDimension ||
        settings.height > kMaximumEditorWindowDimension) {
        return setError(error, "window dimensions are outside the supported range");
    }
    if (settings.x < -kMaximumStoredWindowPosition ||
        settings.x > kMaximumStoredWindowPosition ||
        settings.y < -kMaximumStoredWindowPosition ||
        settings.y > kMaximumStoredWindowPosition) {
        return setError(error, "window position is outside the supported range");
    }
    if (settings.hierarchyWidth < kMinimumEditorHierarchyWidth ||
        settings.hierarchyWidth > kMaximumEditorHierarchyWidth ||
        settings.inspectorWidth < kMinimumEditorInspectorWidth ||
        settings.inspectorWidth > kMaximumEditorInspectorWidth) {
        return setError(error, "editor panel widths are outside the supported range");
    }
    error.clear();
    return true;
}

WindowSettings fitWindowSettingsToWorkArea(
    const WindowSettings& settings,
    const WindowWorkArea& workArea) {
    WindowSettings fitted = settings;
    std::string validationError;
    if (!validateWindowSettings(fitted, validationError)) {
        fitted = WindowSettings{};
    }
    if (workArea.width <= 0 || workArea.height <= 0) {
        return fitted;
    }

    const int minimumWidth = (std::min)(
        kMinimumEditorWindowWidth, workArea.width);
    const int minimumHeight = (std::min)(
        kMinimumEditorWindowHeight, workArea.height);
    fitted.width = std::clamp(fitted.width, minimumWidth, workArea.width);
    fitted.height = std::clamp(fitted.height, minimumHeight, workArea.height);
    if (!fitted.hasPosition) {
        fitted.x = workArea.x + (workArea.width - fitted.width) / 2;
        fitted.y = workArea.y + (workArea.height - fitted.height) / 2;
    } else {
        const std::int64_t maximumX =
            static_cast<std::int64_t>(workArea.x) + workArea.width - fitted.width;
        const std::int64_t maximumY =
            static_cast<std::int64_t>(workArea.y) + workArea.height - fitted.height;
        fitted.x = static_cast<int>(std::clamp<std::int64_t>(
            fitted.x, workArea.x, maximumX));
        fitted.y = static_cast<int>(std::clamp<std::int64_t>(
            fitted.y, workArea.y, maximumY));
    }
    fitted.hasPosition = true;
    return fitted;
}

std::size_t windowWorkAreaIndexForSettings(
    const WindowSettings& settings,
    const std::vector<WindowWorkArea>& workAreas,
    std::size_t fallbackIndex) {
    if (workAreas.empty()) {
        return 0;
    }

    const std::size_t fallback = (std::min)(
        fallbackIndex, workAreas.size() - 1);
    if (!settings.hasPosition || settings.width <= 0 ||
        settings.height <= 0) {
        return fallback;
    }

    const std::int64_t windowLeft = settings.x;
    const std::int64_t windowTop = settings.y;
    const std::int64_t windowRight = windowLeft + settings.width;
    const std::int64_t windowBottom = windowTop + settings.height;
    const auto overlapArea = [&](const WindowWorkArea& area) {
        if (area.width <= 0 || area.height <= 0) {
            return std::int64_t{0};
        }
        const std::int64_t areaRight =
            static_cast<std::int64_t>(area.x) + area.width;
        const std::int64_t areaBottom =
            static_cast<std::int64_t>(area.y) + area.height;
        const std::int64_t overlapWidth = (std::max)(
            std::int64_t{0},
            (std::min)(windowRight, areaRight) -
                (std::max)(windowLeft, static_cast<std::int64_t>(area.x)));
        const std::int64_t overlapHeight = (std::max)(
            std::int64_t{0},
            (std::min)(windowBottom, areaBottom) -
                (std::max)(windowTop, static_cast<std::int64_t>(area.y)));
        return overlapWidth * overlapHeight;
    };

    std::size_t selected = fallback;
    std::int64_t largestOverlap = overlapArea(workAreas[fallback]);
    for (std::size_t index = 0; index < workAreas.size(); ++index) {
        const std::int64_t overlap = overlapArea(workAreas[index]);
        if (overlap > largestOverlap) {
            selected = index;
            largestOverlap = overlap;
        }
    }
    return largestOverlap > 0 ? selected : fallback;
}

WindowSettingsLoadResult loadWindowSettings(
    const std::filesystem::path& path) {
    WindowSettingsLoadResult result;
    if (path.empty()) {
        result.error = "window settings path is empty";
        return result;
    }

    std::error_code filesystemError;
    if (!std::filesystem::exists(path, filesystemError)) {
        if (filesystemError) {
            result.error = "failed to inspect window settings: " +
                filesystemError.message();
        }
        return result;
    }
    const std::uintmax_t fileSize =
        std::filesystem::file_size(path, filesystemError);
    if (filesystemError) {
        result.error = "failed to inspect window settings size: " +
            filesystemError.message();
        return result;
    }
    if (fileSize > kMaximumWindowSettingsFileSize) {
        result.error = "window settings exceed the 64 KiB limit";
        return result;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        result.error = "failed to open window settings";
        return result;
    }
    const Json value = Json::parse(input, nullptr, false);
    if (value.is_discarded() || !value.is_object()) {
        result.error = "window settings contain invalid JSON";
        return result;
    }

    WindowSettings settings;
    if (!readInteger(value, "schema_version", settings.schemaVersion) ||
        !readInteger(value, "x", settings.x) ||
        !readInteger(value, "y", settings.y) ||
        !readInteger(value, "width", settings.width) ||
        !readInteger(value, "height", settings.height) ||
        !readBoolean(value, "has_position", settings.hasPosition) ||
        !readBoolean(value, "fullscreen", settings.fullscreen) ||
        !readBoolean(value, "vsync", settings.vsync)) {
        result.error = "window settings have missing or invalid fields";
        return result;
    }
    if (settings.schemaVersion == 1) {
        settings.schemaVersion = kCurrentWindowSettingsSchemaVersion;
    } else if (settings.schemaVersion == 2) {
        if (!readBoolean(
                value, "hierarchy_expanded", settings.hierarchyExpanded) ||
            !readBoolean(
                value, "inspector_expanded", settings.inspectorExpanded)) {
            result.error = "window settings have missing or invalid panel state";
            return result;
        }
        settings.schemaVersion = kCurrentWindowSettingsSchemaVersion;
    } else if (settings.schemaVersion ==
               kCurrentWindowSettingsSchemaVersion) {
        if (!readBoolean(
                value, "hierarchy_expanded", settings.hierarchyExpanded) ||
            !readBoolean(
                value, "inspector_expanded", settings.inspectorExpanded) ||
            !readInteger(value, "hierarchy_width", settings.hierarchyWidth) ||
            !readInteger(value, "inspector_width", settings.inspectorWidth)) {
            result.error = "window settings have missing or invalid panel layout";
            return result;
        }
    }
    if (!validateWindowSettings(settings, result.error)) {
        return result;
    }

    result.settings = settings;
    result.loaded = true;
    return result;
}

WindowSettingsSaveResult saveWindowSettings(
    const WindowSettings& settings,
    const std::filesystem::path& path) {
    WindowSettingsSaveResult result;
    if (path.empty()) {
        result.error = "window settings path is empty";
        return result;
    }
    if (!validateWindowSettings(settings, result.error)) {
        result.error = "window settings validation failed: " + result.error;
        return result;
    }

    const Json value = {
        {"schema_version", settings.schemaVersion},
        {"x", settings.x},
        {"y", settings.y},
        {"width", settings.width},
        {"height", settings.height},
        {"has_position", settings.hasPosition},
        {"fullscreen", settings.fullscreen},
        {"vsync", settings.vsync},
        {"hierarchy_expanded", settings.hierarchyExpanded},
        {"inspector_expanded", settings.inspectorExpanded},
        {"hierarchy_width", settings.hierarchyWidth},
        {"inspector_width", settings.inspectorWidth}};
    const std::string serialized = value.dump(2);

    std::error_code filesystemError;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(
            path.parent_path(), filesystemError);
        if (filesystemError) {
            result.error = "failed to create window settings directory: " +
                filesystemError.message();
            return result;
        }
    }

    std::filesystem::path temporary = path;
    temporary += ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        result.error = "failed to open temporary window settings";
        return result;
    }
    output.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
    output.put('\n');
    output.close();
    if (!output) {
        result.error = "failed to write temporary window settings";
        std::filesystem::remove(temporary, filesystemError);
        return result;
    }
    if (!replaceFile(temporary, path, result.error)) {
        std::filesystem::remove(temporary, filesystemError);
        return result;
    }

    result.success = true;
    return result;
}

} // namespace core
