#include "editor_layout.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr int kHierarchyHorizontalPadding = 8;
constexpr int kHierarchyTopPadding = 8;
constexpr int kHierarchyActionButtonHeight = 26;
constexpr int kHierarchyActionButtonGap = 8;
constexpr int kHierarchyActionBottomOffset = 58;
constexpr int kToolbarButtonY = 8;
constexpr int kToolbarButtonHeight = 28;
constexpr int kToolbarButtonGap = 8;
constexpr int kInspectorButtonGap = 4;
constexpr int kInspectorButtonHeight = 24;
constexpr int kInspectorButtonRowGap = 6;
constexpr int kPanelTogglePadding = 4;

void reducePanelWidthsToFit(
    int maximumWidth,
    bool hierarchyExpanded,
    bool inspectorExpanded,
    int& hierarchyWidth,
    int& inspectorWidth) {
    const int hierarchyFloor = hierarchyExpanded
        ? core::kMinimumEditorHierarchyWidth
        : core::kEditorCollapsedPanelWidth;
    const int inspectorFloor = inspectorExpanded
        ? core::kMinimumEditorInspectorWidth
        : core::kEditorCollapsedPanelWidth;
    int overflow = hierarchyWidth + inspectorWidth - maximumWidth;
    if (overflow <= 0) {
        return;
    }

    const int hierarchyFlexible = (std::max)(
        0, hierarchyWidth - hierarchyFloor);
    const int inspectorFlexible = (std::max)(
        0, inspectorWidth - inspectorFloor);
    const int totalFlexible = hierarchyFlexible + inspectorFlexible;
    if (totalFlexible > 0) {
        int hierarchyReduction = static_cast<int>(
            (static_cast<long long>(overflow) * hierarchyFlexible +
             totalFlexible / 2) /
            totalFlexible);
        hierarchyReduction = std::clamp(
            hierarchyReduction, 0, hierarchyFlexible);
        int inspectorReduction = (std::min)(
            inspectorFlexible, overflow - hierarchyReduction);
        const int remaining = overflow - hierarchyReduction - inspectorReduction;
        hierarchyReduction += (std::min)(
            remaining, hierarchyFlexible - hierarchyReduction);
        hierarchyWidth -= hierarchyReduction;
        inspectorWidth -= inspectorReduction;
        overflow -= hierarchyReduction + inspectorReduction;
    }

    if (overflow <= 0) {
        return;
    }

    // On very small displays, shrink expanded content before collapsed rails.
    const int hierarchyCompressible = hierarchyExpanded ? hierarchyWidth : 0;
    const int inspectorCompressible = inspectorExpanded ? inspectorWidth : 0;
    const int totalCompressible = hierarchyCompressible + inspectorCompressible;
    if (totalCompressible > 0) {
        int hierarchyReduction = static_cast<int>(
            (static_cast<long long>(overflow) * hierarchyCompressible +
             totalCompressible / 2) /
            totalCompressible);
        hierarchyReduction = std::clamp(
            hierarchyReduction, 0, hierarchyCompressible);
        int inspectorReduction = (std::min)(
            inspectorCompressible, overflow - hierarchyReduction);
        const int remaining = overflow - hierarchyReduction - inspectorReduction;
        hierarchyReduction += (std::min)(
            remaining, hierarchyCompressible - hierarchyReduction);
        hierarchyWidth -= hierarchyReduction;
        inspectorWidth -= inspectorReduction;
        overflow -= hierarchyReduction + inspectorReduction;
    }

    if (overflow > 0) {
        const int hierarchyReduction = (std::min)(overflow, hierarchyWidth);
        hierarchyWidth -= hierarchyReduction;
        overflow -= hierarchyReduction;
        inspectorWidth = (std::max)(0, inspectorWidth - overflow);
    }
}

std::optional<int> clampedRoundedWidth(
    double desired,
    int minimum,
    int maximum) {
    if (!std::isfinite(desired)) {
        return std::nullopt;
    }
    if (desired <= static_cast<double>(minimum)) {
        return minimum;
    }
    if (desired >= static_cast<double>(maximum)) {
        return maximum;
    }
    return std::clamp(
        static_cast<int>(std::lround(desired)), minimum, maximum);
}

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

EditorLayout calculateEditorLayout(
    int width,
    int height,
    bool hierarchyExpanded,
    bool inspectorExpanded,
    int desiredHierarchyWidth,
    int desiredInspectorWidth) {
    EditorLayout layout;
    if (width <= 0 || height <= 0) {
        return layout;
    }

    layout.width = width;
    layout.height = height;
    layout.hierarchyExpanded = hierarchyExpanded;
    layout.inspectorExpanded = inspectorExpanded;

    const int toolbarHeight = (std::min)(kEditorToolbarHeight, height);
    const int statusHeight = (std::min)(
        kEditorStatusBarHeight, (std::max)(0, height - toolbarHeight));
    const int workspaceHeight = (std::max)(0, height - toolbarHeight - statusHeight);

    int hierarchyWidth = hierarchyExpanded
        ? std::clamp(
              desiredHierarchyWidth,
              kMinimumEditorHierarchyWidth,
              kMaximumEditorHierarchyWidth)
        : kEditorCollapsedPanelWidth;
    int inspectorWidth = inspectorExpanded
        ? std::clamp(
              desiredInspectorWidth,
              kMinimumEditorInspectorWidth,
              kMaximumEditorInspectorWidth)
        : kEditorCollapsedPanelWidth;
    const int maximumPanelWidth = (std::max)(
        0, width - kEditorMinimumViewportWidth -
               2 * kEditorPanelSeparatorWidth);
    reducePanelWidthsToFit(
        maximumPanelWidth,
        hierarchyExpanded,
        inspectorExpanded,
        hierarchyWidth,
        inspectorWidth);

    layout.toolbar = {0, 0, width, toolbarHeight};
    layout.statusBar = {0, height - statusHeight, width, statusHeight};
    layout.hierarchy = {0, toolbarHeight, hierarchyWidth, workspaceHeight};
    layout.viewport = {
        hierarchyWidth + kEditorPanelSeparatorWidth,
        toolbarHeight,
        (std::max)(0, width - hierarchyWidth - inspectorWidth -
                           2 * kEditorPanelSeparatorWidth),
        workspaceHeight};
    layout.inspector = {
        width - inspectorWidth, toolbarHeight, inspectorWidth, workspaceHeight};

    const bool panelWidthsResizable =
        (!hierarchyExpanded ||
         hierarchyWidth >= kMinimumEditorHierarchyWidth) &&
        (!inspectorExpanded ||
         inspectorWidth >= kMinimumEditorInspectorWidth);
    const int hierarchyMaximumAvailable = width - inspectorWidth -
        2 * kEditorPanelSeparatorWidth - kEditorMinimumViewportWidth;
    if (panelWidthsResizable && hierarchyExpanded &&
        hierarchyMaximumAvailable > kMinimumEditorHierarchyWidth) {
        const int boundary = layout.hierarchy.x + layout.hierarchy.width;
        layout.hierarchySplitter = {
            boundary - kEditorPanelSplitterHitWidth / 2,
            toolbarHeight,
            kEditorPanelSplitterHitWidth,
            workspaceHeight};
    }
    const int inspectorMaximumAvailable = width - hierarchyWidth -
        2 * kEditorPanelSeparatorWidth - kEditorMinimumViewportWidth;
    if (panelWidthsResizable && inspectorExpanded &&
        inspectorMaximumAvailable > kMinimumEditorInspectorWidth) {
        layout.inspectorSplitter = {
            layout.inspector.x - kEditorPanelSplitterHitWidth / 2,
            toolbarHeight,
            kEditorPanelSplitterHitWidth,
            workspaceHeight};
    }

    const int hierarchyHeaderHeight = (std::min)(
        kEditorPanelHeaderHeight, layout.hierarchy.height);
    layout.hierarchyHeader = {
        layout.hierarchy.x, layout.hierarchy.y,
        layout.hierarchy.width, hierarchyHeaderHeight};

    const int hierarchyToggleWidth = (std::min)(
        26,
        (std::max)(0, layout.hierarchyHeader.width -
                          2 * kPanelTogglePadding));
    const int hierarchyToggleHeight = (std::min)(
        26,
        (std::max)(0, layout.hierarchyHeader.height -
                          2 * kPanelTogglePadding));
    layout.hierarchyToggleButton = {
        layout.hierarchyHeader.x + layout.hierarchyHeader.width -
            kPanelTogglePadding - hierarchyToggleWidth,
        layout.hierarchyHeader.y + kPanelTogglePadding,
        hierarchyToggleWidth,
        hierarchyToggleHeight};

    if (hierarchyExpanded) {
        const int hierarchyBottom =
            layout.hierarchy.y + layout.hierarchy.height;
        const int hierarchyActionY = (std::max)(
            layout.hierarchy.y + hierarchyHeaderHeight,
            hierarchyBottom - kHierarchyActionBottomOffset);
        const int hierarchyActionWidth = (std::max)(
            0,
            (layout.hierarchy.width - 2 * kHierarchyHorizontalPadding -
             kHierarchyActionButtonGap) /
                2);
        layout.hierarchyDuplicateButton = {
            layout.hierarchy.x + kHierarchyHorizontalPadding,
            hierarchyActionY,
            hierarchyActionWidth,
            kHierarchyActionButtonHeight};
        layout.hierarchyDeleteButton = {
            layout.hierarchyDuplicateButton.x + hierarchyActionWidth +
                kHierarchyActionButtonGap,
            hierarchyActionY,
            hierarchyActionWidth,
            kHierarchyActionButtonHeight};

        const int hierarchyListY =
            layout.hierarchy.y + hierarchyHeaderHeight + kHierarchyTopPadding;
        layout.hierarchyList = {
            layout.hierarchy.x + kHierarchyHorizontalPadding,
            hierarchyListY,
            (std::max)(
                0,
                layout.hierarchy.width - 2 * kHierarchyHorizontalPadding),
            (std::max)(
                0,
                hierarchyActionY - kHierarchyTopPadding - hierarchyListY)};
    }

    const int inspectorHeaderHeight = (std::min)(
        kEditorPanelHeaderHeight, layout.inspector.height);
    layout.inspectorHeader = {
        layout.inspector.x, layout.inspector.y,
        layout.inspector.width, inspectorHeaderHeight};
    const int inspectorToggleWidth = (std::min)(
        26,
        (std::max)(0, layout.inspectorHeader.width -
                          2 * kPanelTogglePadding));
    const int inspectorToggleHeight = (std::min)(
        26,
        (std::max)(0, layout.inspectorHeader.height -
                          2 * kPanelTogglePadding));
    layout.inspectorToggleButton = {
        layout.inspectorHeader.x + kPanelTogglePadding,
        layout.inspectorHeader.y + kPanelTogglePadding,
        inspectorToggleWidth,
        inspectorToggleHeight};
    if (inspectorExpanded) {
        layout.inspectorContent = {
            layout.inspector.x + 12,
            layout.inspector.y + inspectorHeaderHeight + 10,
            (std::max)(0, layout.inspector.width - 24),
            (std::max)(
                0,
                layout.inspector.height - inspectorHeaderHeight - 20)};
    }

    const int modalWidth = (std::min)(
        520, (std::max)(0, layout.viewport.width - 48));
    const int modalHeight = (std::min)(
        500, (std::max)(0, layout.viewport.height - 48));
    layout.modalOverlay = {
        layout.viewport.x + (layout.viewport.width - modalWidth) / 2,
        layout.viewport.y + (layout.viewport.height - modalHeight) / 2,
        modalWidth,
        modalHeight};

    int buttonX = 172;
    layout.createButton = toolbarButton(buttonX, 88);
    layout.saveButton = toolbarButton(buttonX, 72);
    layout.loadButton = toolbarButton(buttonX, 72);
    layout.gridButton = toolbarButton(buttonX, 82);
    layout.assetsButton = toolbarButton(buttonX, 88);
    layout.benchmarkButton = toolbarButton(buttonX, 104);

    if (layout.inspectorContent.valid()) {
        const int controlsTop = (std::max)(
            layout.inspectorContent.y,
            (std::min)(
                layout.inspectorContent.y + 300,
                layout.inspector.y + layout.inspector.height - 150));
        const int moveButtonWidth = (std::max)(
            0,
            (layout.inspectorContent.width - 5 * kInspectorButtonGap) / 6);
        int moveButtonX = layout.inspectorContent.x;
        for (core::EditorRect& button : layout.inspectorMoveButtons) {
            button = {
                moveButtonX, controlsTop,
                moveButtonWidth, kInspectorButtonHeight};
            moveButtonX += moveButtonWidth + kInspectorButtonGap;
        }

        const int rotateScaleY =
            controlsTop + kInspectorButtonHeight + kInspectorButtonRowGap;
        const int rotateScaleButtonWidth = (std::max)(
            0,
            (layout.inspectorContent.width - 3 * kInspectorButtonGap) / 4);
        int rotateScaleX = layout.inspectorContent.x;
        for (core::EditorRect& button : layout.inspectorRotateScaleButtons) {
            button = {
                rotateScaleX, rotateScaleY,
                rotateScaleButtonWidth, kInspectorButtonHeight};
            rotateScaleX += rotateScaleButtonWidth + kInspectorButtonGap;
        }

        const int snapResetY =
            rotateScaleY + kInspectorButtonHeight + kInspectorButtonRowGap;
        const int snapResetButtonWidth = (std::max)(
            0,
            (layout.inspectorContent.width - kInspectorButtonGap) / 2);
        layout.inspectorSnapResetButtons[0] = {
            layout.inspectorContent.x, snapResetY,
            snapResetButtonWidth, kInspectorButtonHeight};
        layout.inspectorSnapResetButtons[1] = {
            layout.inspectorContent.x + snapResetButtonWidth +
                kInspectorButtonGap,
            snapResetY, snapResetButtonWidth, kInspectorButtonHeight};
    }
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

EditorHierarchyAction editorHierarchyActionAt(
    const EditorLayout& layout,
    EditorPoint point) {
    if (layout.hierarchyDuplicateButton.contains(point)) {
        return EditorHierarchyAction::DuplicateObject;
    }
    if (layout.hierarchyDeleteButton.contains(point)) {
        return EditorHierarchyAction::DeleteObject;
    }
    return EditorHierarchyAction::None;
}

EditorPanelAction editorPanelActionAt(
    const EditorLayout& layout,
    EditorPoint point) {
    if (layout.hierarchyToggleButton.contains(point)) {
        return EditorPanelAction::ToggleHierarchy;
    }
    if (layout.inspectorToggleButton.contains(point)) {
        return EditorPanelAction::ToggleInspector;
    }
    return EditorPanelAction::None;
}

EditorPanelSplitter editorPanelSplitterAt(
    const EditorLayout& layout,
    EditorPoint point) {
    if (layout.hierarchySplitter.contains(point)) {
        return EditorPanelSplitter::Hierarchy;
    }
    if (layout.inspectorSplitter.contains(point)) {
        return EditorPanelSplitter::Inspector;
    }
    return EditorPanelSplitter::None;
}

std::optional<int> resizedEditorPanelWidth(
    const EditorLayout& layout,
    EditorPanelSplitter splitter,
    EditorPoint point) {
    if (!std::isfinite(point.x)) {
        return std::nullopt;
    }

    if (splitter == EditorPanelSplitter::Hierarchy &&
        layout.hierarchySplitter.valid()) {
        const int maximum = (std::min)(
            kMaximumEditorHierarchyWidth,
            layout.width - layout.inspector.width -
                2 * kEditorPanelSeparatorWidth - kEditorMinimumViewportWidth);
        if (maximum < kMinimumEditorHierarchyWidth) {
            return std::nullopt;
        }
        return clampedRoundedWidth(
            point.x - static_cast<double>(layout.hierarchy.x),
            kMinimumEditorHierarchyWidth,
            maximum);
    }

    if (splitter == EditorPanelSplitter::Inspector &&
        layout.inspectorSplitter.valid()) {
        const int maximum = (std::min)(
            kMaximumEditorInspectorWidth,
            layout.width - layout.hierarchy.width -
                2 * kEditorPanelSeparatorWidth - kEditorMinimumViewportWidth);
        if (maximum < kMinimumEditorInspectorWidth) {
            return std::nullopt;
        }
        return clampedRoundedWidth(
            static_cast<double>(layout.width) - point.x,
            kMinimumEditorInspectorWidth,
            maximum);
    }
    return std::nullopt;
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

std::optional<ObjectTransformCommand> editorInspectorTransformAt(
    const EditorLayout& layout,
    EditorPoint point) {
    constexpr std::array<ObjectTransformCommand, 6> moveCommands = {
        ObjectTransformCommand::MoveNegativeX,
        ObjectTransformCommand::MovePositiveX,
        ObjectTransformCommand::MoveNegativeY,
        ObjectTransformCommand::MovePositiveY,
        ObjectTransformCommand::MoveNegativeZ,
        ObjectTransformCommand::MovePositiveZ};
    for (std::size_t index = 0; index < moveCommands.size(); ++index) {
        if (layout.inspectorMoveButtons[index].contains(point)) {
            return moveCommands[index];
        }
    }

    constexpr std::array<ObjectTransformCommand, 4> rotateScaleCommands = {
        ObjectTransformCommand::RotateNegativeY,
        ObjectTransformCommand::RotatePositiveY,
        ObjectTransformCommand::ScaleDown,
        ObjectTransformCommand::ScaleUp};
    for (std::size_t index = 0;
         index < rotateScaleCommands.size(); ++index) {
        if (layout.inspectorRotateScaleButtons[index].contains(point)) {
            return rotateScaleCommands[index];
        }
    }

    if (layout.inspectorSnapResetButtons[0].contains(point)) {
        return ObjectTransformCommand::Snap;
    }
    if (layout.inspectorSnapResetButtons[1].contains(point)) {
        return ObjectTransformCommand::Reset;
    }
    return std::nullopt;
}

} // namespace core
