#include "editor_layout.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr int kMinimumViewportWidth = 320;
constexpr int kHierarchyMinimumWidth = 200;
constexpr int kHierarchyMaximumWidth = 260;
constexpr int kInspectorMinimumWidth = 260;
constexpr int kInspectorMaximumWidth = 340;
constexpr int kPanelSeparatorWidth = 1;
constexpr int kHierarchyHorizontalPadding = 8;
constexpr int kHierarchyTopPadding = 8;
constexpr int kToolbarButtonY = 8;
constexpr int kToolbarButtonHeight = 28;
constexpr int kToolbarButtonGap = 8;

core::EditorRect toolbarButton(int& x, int width) {
    const core::EditorRect button{x, kToolbarButtonY, width, kToolbarButtonHeight};
    x += width + kToolbarButtonGap;
    return button;
}

} // namespace

namespace core {

bool EditorRect::contains(EditorPoint point) const {
    return valid() && std::isfinite(point.x) && std::isfinite(point.y) &&
        point.x >= static_cast<double>(x) &&
        point.y >= static_cast<double>(y) &&
        point.x < static_cast<double>(x + width) &&
        point.y < static_cast<double>(y + height);
}

bool EditorRect::valid() const {
    return width > 0 && height > 0;
}

EditorLayout calculateEditorLayout(int width, int height) {
    EditorLayout layout;
    if (width <= 0 || height <= 0) {
        return layout;
    }

    layout.width = width;
    layout.height = height;

    const int toolbarHeight = (std::min)(kEditorToolbarHeight, height);
    const int statusHeight = (std::min)(
        kEditorStatusBarHeight, (std::max)(0, height - toolbarHeight));
    const int workspaceHeight = (std::max)(0, height - toolbarHeight - statusHeight);

    int hierarchyWidth = std::clamp(
        width * 18 / 100, kHierarchyMinimumWidth, kHierarchyMaximumWidth);
    int inspectorWidth = std::clamp(
        width * 23 / 100, kInspectorMinimumWidth, kInspectorMaximumWidth);
    const int maximumPanelWidth = (std::max)(
        0, width - kMinimumViewportWidth - 2 * kPanelSeparatorWidth);
    const int desiredPanelWidth = hierarchyWidth + inspectorWidth;
    if (desiredPanelWidth > maximumPanelWidth && desiredPanelWidth > 0) {
        const double scale = static_cast<double>(maximumPanelWidth) /
            static_cast<double>(desiredPanelWidth);
        hierarchyWidth = static_cast<int>(std::floor(hierarchyWidth * scale));
        inspectorWidth = maximumPanelWidth - hierarchyWidth;
    }

    layout.toolbar = {0, 0, width, toolbarHeight};
    layout.statusBar = {0, height - statusHeight, width, statusHeight};
    layout.hierarchy = {0, toolbarHeight, hierarchyWidth, workspaceHeight};
    layout.viewport = {
        hierarchyWidth + kPanelSeparatorWidth,
        toolbarHeight,
        (std::max)(0, width - hierarchyWidth - inspectorWidth -
                           2 * kPanelSeparatorWidth),
        workspaceHeight};
    layout.inspector = {
        width - inspectorWidth, toolbarHeight, inspectorWidth, workspaceHeight};

    const int hierarchyHeaderHeight = (std::min)(
        kEditorPanelHeaderHeight, layout.hierarchy.height);
    layout.hierarchyHeader = {
        layout.hierarchy.x, layout.hierarchy.y,
        layout.hierarchy.width, hierarchyHeaderHeight};
    layout.hierarchyList = {
        layout.hierarchy.x + kHierarchyHorizontalPadding,
        layout.hierarchy.y + hierarchyHeaderHeight + kHierarchyTopPadding,
        (std::max)(0, layout.hierarchy.width - 2 * kHierarchyHorizontalPadding),
        (std::max)(0, layout.hierarchy.height - hierarchyHeaderHeight -
                           2 * kHierarchyTopPadding)};

    const int inspectorHeaderHeight = (std::min)(
        kEditorPanelHeaderHeight, layout.inspector.height);
    layout.inspectorHeader = {
        layout.inspector.x, layout.inspector.y,
        layout.inspector.width, inspectorHeaderHeight};
    layout.inspectorContent = {
        layout.inspector.x + 12,
        layout.inspector.y + inspectorHeaderHeight + 10,
        (std::max)(0, layout.inspector.width - 24),
        (std::max)(0, layout.inspector.height - inspectorHeaderHeight - 20)};

    int buttonX = 172;
    layout.createButton = toolbarButton(buttonX, 88);
    layout.saveButton = toolbarButton(buttonX, 72);
    layout.loadButton = toolbarButton(buttonX, 72);
    layout.gridButton = toolbarButton(buttonX, 82);
    layout.assetsButton = toolbarButton(buttonX, 88);
    layout.benchmarkButton = toolbarButton(buttonX, 104);
    return layout;
}

EditorPoint mapWindowPointToFramebuffer(
    EditorPoint point,
    int windowWidth,
    int windowHeight,
    int framebufferWidth,
    int framebufferHeight) {
    if (windowWidth <= 0 || windowHeight <= 0 ||
        framebufferWidth <= 0 || framebufferHeight <= 0 ||
        !std::isfinite(point.x) || !std::isfinite(point.y)) {
        return {};
    }
    return {
        point.x * static_cast<double>(framebufferWidth) /
            static_cast<double>(windowWidth),
        point.y * static_cast<double>(framebufferHeight) /
            static_cast<double>(windowHeight)};
}

EditorToolbarAction editorToolbarActionAt(
    const EditorLayout& layout,
    EditorPoint point) {
    if (layout.createButton.contains(point)) {
        return EditorToolbarAction::CreateObject;
    }
    if (layout.saveButton.contains(point)) {
        return EditorToolbarAction::SaveScene;
    }
    if (layout.loadButton.contains(point)) {
        return EditorToolbarAction::LoadScene;
    }
    if (layout.gridButton.contains(point)) {
        return EditorToolbarAction::ToggleGrid;
    }
    if (layout.assetsButton.contains(point)) {
        return EditorToolbarAction::ToggleAssets;
    }
    if (layout.benchmarkButton.contains(point)) {
        return EditorToolbarAction::ToggleBenchmark;
    }
    return EditorToolbarAction::None;
}

std::size_t editorHierarchyVisibleRowCount(const EditorLayout& layout) {
    if (!layout.hierarchyList.valid()) {
        return 0;
    }
    return static_cast<std::size_t>(
        layout.hierarchyList.height / kEditorHierarchyRowHeight);
}

std::size_t firstVisibleEditorObject(
    const EditorLayout& layout,
    std::size_t objectCount,
    int selectedObject) {
    const std::size_t visibleRows = editorHierarchyVisibleRowCount(layout);
    if (visibleRows == 0 || objectCount <= visibleRows || selectedObject < 0) {
        return 0;
    }
    const std::size_t selected = (std::min)(
        static_cast<std::size_t>(selectedObject), objectCount - 1);
    return selected >= visibleRows ? selected - visibleRows + 1 : 0;
}

EditorRect editorHierarchyRowRect(
    const EditorLayout& layout,
    std::size_t visibleRow) {
    const std::size_t visibleRows = editorHierarchyVisibleRowCount(layout);
    if (visibleRow >= visibleRows) {
        return {};
    }
    return {
        layout.hierarchyList.x,
        layout.hierarchyList.y +
            static_cast<int>(visibleRow) * kEditorHierarchyRowHeight,
        layout.hierarchyList.width,
        kEditorHierarchyRowHeight};
}

std::optional<std::size_t> editorHierarchyObjectAt(
    const EditorLayout& layout,
    EditorPoint point,
    std::size_t objectCount,
    std::size_t firstVisibleObject) {
    if (!layout.hierarchyList.contains(point) || objectCount == 0) {
        return std::nullopt;
    }
    const int relativeY = static_cast<int>(point.y) - layout.hierarchyList.y;
    const std::size_t visibleRow = static_cast<std::size_t>(
        relativeY / kEditorHierarchyRowHeight);
    const std::size_t objectIndex = firstVisibleObject + visibleRow;
    if (visibleRow >= editorHierarchyVisibleRowCount(layout) ||
        objectIndex >= objectCount) {
        return std::nullopt;
    }
    return objectIndex;
}

} // namespace core
