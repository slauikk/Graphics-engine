#ifndef CORE_WINDOW_SETTINGS_H
#define CORE_WINDOW_SETTINGS_H

#include "editor_layout.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace core {

inline constexpr int kCurrentWindowSettingsSchemaVersion = 3;
inline constexpr int kDefaultEditorWindowWidth = 1280;
inline constexpr int kDefaultEditorWindowHeight = 720;
inline constexpr int kMinimumEditorWindowWidth = 800;
inline constexpr int kMinimumEditorWindowHeight = 600;
inline constexpr int kMaximumEditorWindowDimension = 16'384;

struct WindowSettings {
    int schemaVersion = kCurrentWindowSettingsSchemaVersion;
    int x = 0;
    int y = 0;
    int width = kDefaultEditorWindowWidth;
    int height = kDefaultEditorWindowHeight;
    bool hasPosition = false;
    bool fullscreen = false;
    bool vsync = true;
    bool hierarchyExpanded = true;
    bool inspectorExpanded = true;
    int hierarchyWidth = kDefaultEditorHierarchyWidth;
    int inspectorWidth = kDefaultEditorInspectorWidth;
};

struct WindowWorkArea {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct WindowSettingsLoadResult {
    WindowSettings settings;
    bool loaded = false;
    std::string error;
};

struct WindowSettingsSaveResult {
    bool success = false;
    std::string error;
};

bool validateWindowSettings(
    const WindowSettings& settings,
    std::string& error);

WindowSettings fitWindowSettingsToWorkArea(
    const WindowSettings& settings,
    const WindowWorkArea& workArea);

std::size_t windowWorkAreaIndexForSettings(
    const WindowSettings& settings,
    const std::vector<WindowWorkArea>& workAreas,
    std::size_t fallbackIndex);

WindowSettingsLoadResult loadWindowSettings(
    const std::filesystem::path& path);

WindowSettingsSaveResult saveWindowSettings(
    const WindowSettings& settings,
    const std::filesystem::path& path);

} // namespace core

#endif // CORE_WINDOW_SETTINGS_H
