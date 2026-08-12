#ifndef CORE_EDITOR_LAYOUT_H
#define CORE_EDITOR_LAYOUT_H

#include "editor_transform.h"

#include <array>
#include <cstddef>
#include <optional>

namespace core {

inline constexpr int kEditorMenuBarHeight = 24;
inline constexpr int kEditorToolbarHeight = 44;
inline constexpr int kEditorTopChromeHeight =
    kEditorMenuBarHeight + kEditorToolbarHeight;
inline constexpr int kEditorViewportHeaderHeight = 30;
inline constexpr int kEditorContentBrowserHeight = 220;
inline constexpr int kEditorContentBrowserHeaderHeight = 30;
inline constexpr int kEditorStatusBarHeight = 24;
inline constexpr int kEditorPanelHeaderHeight = 34;
inline constexpr int kEditorHierarchyRowHeight = 24;
inline constexpr int kEditorCollapsedPanelWidth = 34;
inline constexpr int kEditorMinimumViewportWidth = 320;
inline constexpr int kDefaultEditorHierarchyWidth = 230;
inline constexpr int kMinimumEditorHierarchyWidth = 180;
inline constexpr int kMaximumEditorHierarchyWidth = 480;
inline constexpr int kDefaultEditorInspectorWidth = 294;
inline constexpr int kMinimumEditorInspectorWidth = 240;
inline constexpr int kMaximumEditorInspectorWidth = 560;
inline constexpr int kEditorPanelSeparatorWidth = 1;
inline constexpr int kEditorPanelSplitterHitWidth = 8;
inline constexpr std::size_t kEditorMenuCount = 4;
inline constexpr std::size_t kEditorMenuMaximumItems = 5;

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

enum class EditorHierarchyAction {
    None,
    DuplicateObject,
    DeleteObject
};

enum class EditorPanelAction {
    None,
    ToggleHierarchy,
    ToggleInspector
};

enum class EditorPanelSplitter {
    None,
    Hierarchy,
    Inspector
};

enum class EditorCloseDialogAction {
    None,
    SaveAndExit,
    DiscardAndExit,
    Cancel
};

enum class EditorMenu {
    None,
    File,
    Edit,
    View,
    Window
};

enum class EditorMenuAction {
    None,
    SaveScene,
    LoadScene,
    ExitApplication,
    Undo,
    Redo,
    DuplicateObject,
    DeleteObject,
    ToggleGrid,
    ToggleGpuInfo,
    ToggleVsync,
    ToggleFullscreen,
    OpenAssets,
    ToggleHierarchy,
    ToggleInspector
};

struct EditorMenuPopup {
    EditorRect bounds;
    std::array<EditorRect, kEditorMenuMaximumItems> items;
    std::size_t itemCount = 0;
};

struct EditorLayout {
    int width = 0;
    int height = 0;
    bool hierarchyExpanded = true;
    bool inspectorExpanded = true;
    bool contentBrowserOpen = false;
    EditorRect menuBar;
    EditorRect toolbar;
    EditorRect hierarchy;
    EditorRect hierarchyHeader;
    EditorRect hierarchyToggleButton;
    EditorRect hierarchyList;
    EditorRect hierarchyDuplicateButton;
    EditorRect hierarchyDeleteButton;
    EditorRect hierarchySplitter;
    EditorRect viewport;
    EditorRect inspector;
    EditorRect inspectorHeader;
    EditorRect inspectorToggleButton;
    EditorRect inspectorContent;
    EditorRect inspectorSplitter;
    EditorRect viewportFrame;
    EditorRect viewportHeader;
    EditorRect contentBrowser;
    EditorRect contentBrowserHeader;
    EditorRect contentBrowserContent;
    EditorRect statusBar;
    EditorRect modalOverlay;
    EditorRect closeSaveButton;
    EditorRect closeDiscardButton;
    EditorRect closeCancelButton;
    EditorRect createButton;
    EditorRect saveButton;
    EditorRect loadButton;
    EditorRect gridButton;
    EditorRect assetsButton;
    EditorRect benchmarkButton;
    std::array<EditorRect, kEditorMenuCount> menuButtons;
    std::array<EditorRect, 6> inspectorMoveButtons;
    std::array<EditorRect, 4> inspectorRotateScaleButtons;
    std::array<EditorRect, 2> inspectorSnapResetButtons;
};

EditorLayout calculateEditorLayout(
    int width,
    int height,
    bool hierarchyExpanded = true,
    bool inspectorExpanded = true,
    int desiredHierarchyWidth = kDefaultEditorHierarchyWidth,
    int desiredInspectorWidth = kDefaultEditorInspectorWidth,
    bool contentBrowserOpen = false);

EditorPoint mapWindowPointToFramebuffer(
    EditorPoint point,
    int windowWidth,
    int windowHeight,
    int framebufferWidth,
    int framebufferHeight);

EditorToolbarAction editorToolbarActionAt(
    const EditorLayout& layout,
    EditorPoint point);

EditorMenu editorMenuAt(
    const EditorLayout& layout,
    EditorPoint point);

EditorMenuPopup calculateEditorMenuPopup(
    const EditorLayout& layout,
    EditorMenu menu);

EditorMenuAction editorMenuAction(
    EditorMenu menu,
    std::size_t index);

EditorMenuAction editorMenuActionAt(
    const EditorLayout& layout,
    EditorMenu menu,
    EditorPoint point);

EditorHierarchyAction editorHierarchyActionAt(
    const EditorLayout& layout,
    EditorPoint point);

EditorPanelAction editorPanelActionAt(
    const EditorLayout& layout,
    EditorPoint point);

EditorPanelSplitter editorPanelSplitterAt(
    const EditorLayout& layout,
    EditorPoint point);

EditorCloseDialogAction editorCloseDialogActionAt(
    const EditorLayout& layout,
    EditorPoint point);

std::optional<int> resizedEditorPanelWidth(
    const EditorLayout& layout,
    EditorPanelSplitter splitter,
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

std::optional<ObjectTransformCommand> editorInspectorTransformAt(
    const EditorLayout& layout,
    EditorPoint point);

} // namespace core

#endif // CORE_EDITOR_LAYOUT_H
