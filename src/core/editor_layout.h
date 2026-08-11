#ifndef CORE_EDITOR_LAYOUT_H
#define CORE_EDITOR_LAYOUT_H

#include <cstddef>
#include <optional>

namespace core {

inline constexpr int kEditorToolbarHeight = 44;
inline constexpr int kEditorStatusBarHeight = 28;
inline constexpr int kEditorPanelHeaderHeight = 34;
inline constexpr int kEditorHierarchyRowHeight = 24;

struct EditorPoint {
    double x = -1.0;
    double y = -1.0;
};

struct EditorRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool contains(EditorPoint point) const;
    bool valid() const;
};

enum class EditorToolbarAction {
    None,
    CreateObject,
    SaveScene,
    LoadScene,
    ToggleGrid,
    ToggleAssets,
    ToggleBenchmark
};

struct EditorLayout {
    int width = 0;
    int height = 0;
    EditorRect toolbar;
    EditorRect hierarchy;
    EditorRect hierarchyHeader;
    EditorRect hierarchyList;
    EditorRect viewport;
    EditorRect inspector;
    EditorRect inspectorHeader;
    EditorRect inspectorContent;
    EditorRect statusBar;
    EditorRect createButton;
    EditorRect saveButton;
    EditorRect loadButton;
    EditorRect gridButton;
    EditorRect assetsButton;
    EditorRect benchmarkButton;
};

EditorLayout calculateEditorLayout(int width, int height);

EditorPoint mapWindowPointToFramebuffer(
    EditorPoint point,
    int windowWidth,
    int windowHeight,
    int framebufferWidth,
    int framebufferHeight);

EditorToolbarAction editorToolbarActionAt(
    const EditorLayout& layout,
    EditorPoint point);

std::size_t editorHierarchyVisibleRowCount(const EditorLayout& layout);

std::size_t firstVisibleEditorObject(
    const EditorLayout& layout,
    std::size_t objectCount,
    int selectedObject);

EditorRect editorHierarchyRowRect(
    const EditorLayout& layout,
    std::size_t visibleRow);

std::optional<std::size_t> editorHierarchyObjectAt(
    const EditorLayout& layout,
    EditorPoint point,
    std::size_t objectCount,
    std::size_t firstVisibleObject);

} // namespace core

#endif // CORE_EDITOR_LAYOUT_H
