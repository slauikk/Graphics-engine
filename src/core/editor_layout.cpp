#include "editor_layout.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr int kHierarchyHorizontalPadding = 8;
constexpr int kHierarchyTopPadding = 8;
constexpr int kHierarchyActionButtonHeight = 26;
constexpr int kHierarchyActionButtonGap = 8;
constexpr int kHierarchyActionBottomOffset = 58;
constexpr int kToolbarButtonY = core::kEditorMenuBarHeight + 8;
constexpr int kToolbarButtonHeight = 28;
constexpr int kToolbarButtonGap = 8;
constexpr int kViewportButtonHorizontalPadding = 8;
constexpr int kViewportButtonVerticalPadding = 4;
constexpr int kViewportButtonGap = 4;
constexpr int kMenuStartX = 132;
constexpr int kMenuPopupWidth = 236;
constexpr int kMenuPopupPadding = 4;
constexpr int kMenuItemHeight = 28;
constexpr int kInspectorButtonGap = 4;
constexpr int kInspectorButtonHeight = 24;
constexpr int kInspectorButtonRowGap = 6;
constexpr int kPanelTogglePadding = 4;
constexpr int kCloseDialogButtonHeight = 32;
constexpr int kCloseDialogButtonGap = 8;
constexpr int kCloseDialogPadding = 24;
constexpr int kCloseDialogCompactButtonHeight = 28;
constexpr int kCloseDialogCompactButtonGap = 6;
constexpr int kCloseDialogCompactPadding = 12;
constexpr int kCloseDialogCompactMinimumButtonWidth = 44;

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

core::EditorRect toolbarButton(int& x, int width, int layoutWidth) {
    const core::EditorRect button = x + width <= layoutWidth
        ? core::EditorRect{x, kToolbarButtonY, width, kToolbarButtonHeight}
        : core::EditorRect{};
    x += width + kToolbarButtonGap;
    return button;
}

core::EditorRect viewportHeaderButton(
    int& x,
    int width,
    const core::EditorRect& header) {
    const int height = (std::max)(
        0, header.height - 2 * kViewportButtonVerticalPadding);
    const int right = header.x + header.width -
        kViewportButtonHorizontalPadding;
    const core::EditorRect button =
        height > 0 && x + width <= right
        ? core::EditorRect{
              x,
              header.y + kViewportButtonVerticalPadding,
              width,
              height}
        : core::EditorRect{};
    x += width + kViewportButtonGap;
    return button;
}

std::size_t editorMenuItemCount(core::EditorMenu menu) {
    switch (menu) {
        case core::EditorMenu::File: return 3;
        case core::EditorMenu::Edit: return 4;
        case core::EditorMenu::View: return 5;
        case core::EditorMenu::Window: return 2;
        case core::EditorMenu::None: return 0;
    }
    return 0;
}

core::EditorMenuAction editorMenuActionAtIndex(
    core::EditorMenu menu,
    std::size_t index) {
    constexpr std::array fileActions = {
        core::EditorMenuAction::SaveScene,
        core::EditorMenuAction::LoadScene,
        core::EditorMenuAction::ExitApplication};
    constexpr std::array editActions = {
        core::EditorMenuAction::Undo,
        core::EditorMenuAction::Redo,
        core::EditorMenuAction::DuplicateObject,
        core::EditorMenuAction::DeleteObject};
    constexpr std::array viewActions = {
        core::EditorMenuAction::ToggleGrid,
        core::EditorMenuAction::ToggleGpuInfo,
        core::EditorMenuAction::ToggleVsync,
        core::EditorMenuAction::ToggleFullscreen,
        core::EditorMenuAction::OpenAssets};
    constexpr std::array windowActions = {
        core::EditorMenuAction::ToggleHierarchy,
        core::EditorMenuAction::ToggleInspector};

    switch (menu) {
        case core::EditorMenu::File:
            return index < fileActions.size()
                ? fileActions[index] : core::EditorMenuAction::None;
        case core::EditorMenu::Edit:
            return index < editActions.size()
                ? editActions[index] : core::EditorMenuAction::None;
        case core::EditorMenu::View:
            return index < viewActions.size()
                ? viewActions[index] : core::EditorMenuAction::None;
        case core::EditorMenu::Window:
            return index < windowActions.size()
                ? windowActions[index] : core::EditorMenuAction::None;
        case core::EditorMenu::None:
            return core::EditorMenuAction::None;
    }
    return core::EditorMenuAction::None;
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
    int desiredInspectorWidth,
    bool contentBrowserOpen) {
    EditorLayout layout;
    if (width <= 0 || height <= 0) {
        return layout;
    }

    layout.width = width;
    layout.height = height;
    layout.hierarchyExpanded = hierarchyExpanded;
    layout.inspectorExpanded = inspectorExpanded;
    layout.contentBrowserOpen = contentBrowserOpen;

    const int topChromeHeight = (std::min)(kEditorTopChromeHeight, height);
    const int menuBarHeight = (std::min)(kEditorMenuBarHeight, topChromeHeight);
    const int toolbarHeight = topChromeHeight - menuBarHeight;
    const int statusHeight = (std::min)(
        kEditorStatusBarHeight, (std::max)(0, height - topChromeHeight));
    const int workspaceHeight = (std::max)(
        0, height - topChromeHeight - statusHeight);

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

    layout.menuBar = {0, 0, width, menuBarHeight};
    layout.toolbar = {0, menuBarHeight, width, toolbarHeight};
    layout.statusBar = {0, height - statusHeight, width, statusHeight};
    layout.hierarchy = {0, topChromeHeight, hierarchyWidth, workspaceHeight};
    const int centerX = hierarchyWidth + kEditorPanelSeparatorWidth;
    const int centerWidth = (std::max)(
        0, width - hierarchyWidth - inspectorWidth -
               2 * kEditorPanelSeparatorWidth);
    constexpr int minimumViewportFrameHeight =
        kEditorViewportHeaderHeight + 120;
    const int maximumContentBrowserHeight = (std::max)(
        0, workspaceHeight - minimumViewportFrameHeight -
               kEditorPanelSeparatorWidth);
    const int contentBrowserHeight = contentBrowserOpen
        ? (std::min)(kEditorContentBrowserHeight, maximumContentBrowserHeight)
        : 0;
    const int contentBrowserSeparator = contentBrowserHeight > 0
        ? kEditorPanelSeparatorWidth
        : 0;
    const int viewportFrameHeight = (std::max)(
        0, workspaceHeight - contentBrowserHeight - contentBrowserSeparator);
    layout.viewportFrame = {
        centerX,
        topChromeHeight,
        centerWidth,
        viewportFrameHeight};
    const int viewportHeaderHeight = (std::min)(
        kEditorViewportHeaderHeight, layout.viewportFrame.height);
    layout.viewportHeader = {
        layout.viewportFrame.x,
        layout.viewportFrame.y,
        layout.viewportFrame.width,
        viewportHeaderHeight};
    int viewportButtonX = layout.viewportHeader.x +
        kViewportButtonHorizontalPadding;
    layout.viewportViewModeButton = viewportHeaderButton(
        viewportButtonX, 68, layout.viewportHeader);
    layout.viewportPostProcessButton = viewportHeaderButton(
        viewportButtonX, 54, layout.viewportHeader);
    layout.viewportMoveButton = viewportHeaderButton(
        viewportButtonX, 54, layout.viewportHeader);
    layout.viewportRotateButton = viewportHeaderButton(
        viewportButtonX, 58, layout.viewportHeader);
    layout.viewportSnapButton = viewportHeaderButton(
        viewportButtonX, 54, layout.viewportHeader);
    layout.viewport = {
        layout.viewportFrame.x,
        layout.viewportFrame.y + viewportHeaderHeight,
        layout.viewportFrame.width,
        (std::max)(0, layout.viewportFrame.height - viewportHeaderHeight)};
    if (contentBrowserHeight > 0) {
        layout.contentBrowser = {
            centerX,
            topChromeHeight + viewportFrameHeight + contentBrowserSeparator,
            centerWidth,
            contentBrowserHeight};
        const int contentBrowserHeaderHeight = (std::min)(
            kEditorContentBrowserHeaderHeight,
            layout.contentBrowser.height);
        layout.contentBrowserHeader = {
            layout.contentBrowser.x,
            layout.contentBrowser.y,
            layout.contentBrowser.width,
            contentBrowserHeaderHeight};
        layout.contentBrowserContent = {
            layout.contentBrowser.x + 12,
            layout.contentBrowser.y + contentBrowserHeaderHeight + 8,
            (std::max)(0, layout.contentBrowser.width - 24),
            (std::max)(
                0,
                layout.contentBrowser.height -
                    contentBrowserHeaderHeight - 16)};
    }
    layout.inspector = {
        width - inspectorWidth, topChromeHeight, inspectorWidth, workspaceHeight};

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
            topChromeHeight,
            kEditorPanelSplitterHitWidth,
            workspaceHeight};
    }
    const int inspectorMaximumAvailable = width - hierarchyWidth -
        2 * kEditorPanelSeparatorWidth - kEditorMinimumViewportWidth;
    if (panelWidthsResizable && inspectorExpanded &&
        inspectorMaximumAvailable > kMinimumEditorInspectorWidth) {
        layout.inspectorSplitter = {
            layout.inspector.x - kEditorPanelSplitterHitWidth / 2,
            topChromeHeight,
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
    constexpr int closeButtonStackHeight =
        3 * kCloseDialogButtonHeight + 2 * kCloseDialogButtonGap;
    if (modalWidth > 2 * kCloseDialogPadding &&
        modalHeight >= closeButtonStackHeight + 2 * kCloseDialogPadding) {
        const int closeButtonX =
            layout.modalOverlay.x + kCloseDialogPadding;
        const int closeButtonWidth =
            modalWidth - 2 * kCloseDialogPadding;
        const int closeButtonY = layout.modalOverlay.y + modalHeight -
            kCloseDialogPadding - closeButtonStackHeight;
        layout.closeSaveButton = {
            closeButtonX, closeButtonY,
            closeButtonWidth, kCloseDialogButtonHeight};
        layout.closeDiscardButton = {
            closeButtonX,
            closeButtonY + kCloseDialogButtonHeight + kCloseDialogButtonGap,
            closeButtonWidth, kCloseDialogButtonHeight};
        layout.closeCancelButton = {
            closeButtonX,
            closeButtonY + 2 * (kCloseDialogButtonHeight + kCloseDialogButtonGap),
            closeButtonWidth, kCloseDialogButtonHeight};
    } else {
        constexpr int compactButtonRowWidth =
            3 * kCloseDialogCompactMinimumButtonWidth +
            2 * kCloseDialogCompactButtonGap;
        const int availableWidth =
            modalWidth - 2 * kCloseDialogCompactPadding;
        if (availableWidth >= compactButtonRowWidth &&
            modalHeight >= kCloseDialogCompactButtonHeight +
                2 * kCloseDialogCompactPadding) {
            const int compactButtonWidth =
                (availableWidth - 2 * kCloseDialogCompactButtonGap) / 3;
            const int compactButtonY =
                layout.modalOverlay.y + modalHeight -
                kCloseDialogCompactPadding - kCloseDialogCompactButtonHeight;
            layout.closeSaveButton = {
                layout.modalOverlay.x + kCloseDialogCompactPadding,
                compactButtonY,
                compactButtonWidth,
                kCloseDialogCompactButtonHeight};
            layout.closeDiscardButton = {
                layout.closeSaveButton.x + compactButtonWidth +
                    kCloseDialogCompactButtonGap,
                compactButtonY,
                compactButtonWidth,
                kCloseDialogCompactButtonHeight};
            layout.closeCancelButton = {
                layout.closeDiscardButton.x + compactButtonWidth +
                    kCloseDialogCompactButtonGap,
                compactButtonY,
                compactButtonWidth,
                kCloseDialogCompactButtonHeight};
        }
    }

    constexpr std::array<int, kEditorMenuCount> menuWidths = {48, 48, 52, 68};
    int menuX = kMenuStartX;
    for (std::size_t index = 0; index < menuWidths.size(); ++index) {
        const int menuWidth = menuWidths[index];
        if (menuX + menuWidth <= width) {
            layout.menuButtons[index] = {
                menuX, 0, menuWidth, menuBarHeight};
        }
        menuX += menuWidth;
    }

    int buttonX = 12;
    layout.createButton = toolbarButton(buttonX, 96, width);
    layout.saveButton = toolbarButton(buttonX, 70, width);
    layout.loadButton = toolbarButton(buttonX, 70, width);
    layout.gridButton = toolbarButton(buttonX, 84, width);
    layout.assetsButton = toolbarButton(buttonX, 96, width);
    layout.benchmarkButton = toolbarButton(buttonX, 100, width);

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

EditorMenu editorMenuAt(
    const EditorLayout& layout,
    EditorPoint point) {
    constexpr std::array menus = {
        EditorMenu::File,
        EditorMenu::Edit,
        EditorMenu::View,
        EditorMenu::Window};
    for (std::size_t index = 0; index < menus.size(); ++index) {
        if (layout.menuButtons[index].contains(point)) {
            return menus[index];
        }
    }
    return EditorMenu::None;
}

EditorMenuPopup calculateEditorMenuPopup(
    const EditorLayout& layout,
    EditorMenu menu) {
    EditorMenuPopup popup;
    if (menu == EditorMenu::None || !layout.menuBar.valid()) {
        return popup;
    }

    const auto menuIndex = static_cast<std::size_t>(menu) - 1;
    if (menuIndex >= layout.menuButtons.size() ||
        !layout.menuButtons[menuIndex].valid()) {
        return popup;
    }
    popup.itemCount = editorMenuItemCount(menu);
    const int popupWidth = (std::min)(kMenuPopupWidth, layout.width);
    const int popupX = std::clamp(
        layout.menuButtons[menuIndex].x,
        0,
        (std::max)(0, layout.width - popupWidth));
    popup.bounds = {
        popupX,
        layout.menuBar.y + layout.menuBar.height,
        popupWidth,
        2 * kMenuPopupPadding +
            static_cast<int>(popup.itemCount) * kMenuItemHeight};
    for (std::size_t index = 0; index < popup.itemCount; ++index) {
        popup.items[index] = {
            popup.bounds.x + kMenuPopupPadding,
            popup.bounds.y + kMenuPopupPadding +
                static_cast<int>(index) * kMenuItemHeight,
            popup.bounds.width - 2 * kMenuPopupPadding,
            kMenuItemHeight};
    }
    return popup;
}

EditorMenuAction editorMenuAction(
    EditorMenu menu,
    std::size_t index) {
    return editorMenuActionAtIndex(menu, index);
}

EditorMenuAction editorMenuActionAt(
    const EditorLayout& layout,
    EditorMenu menu,
    EditorPoint point) {
    const EditorMenuPopup popup = calculateEditorMenuPopup(layout, menu);
    for (std::size_t index = 0; index < popup.itemCount; ++index) {
        if (popup.items[index].contains(point)) {
            return editorMenuAction(menu, index);
        }
    }
    return EditorMenuAction::None;
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

EditorViewportAction editorViewportActionAt(
    const EditorLayout& layout,
    EditorPoint point) {
    if (layout.viewportViewModeButton.contains(point)) {
        return EditorViewportAction::CycleViewMode;
    }
    if (layout.viewportPostProcessButton.contains(point)) {
        return EditorViewportAction::CyclePostProcess;
    }
    if (layout.viewportMoveButton.contains(point)) {
        return EditorViewportAction::SelectTranslate;
    }
    if (layout.viewportRotateButton.contains(point)) {
        return EditorViewportAction::SelectRotate;
    }
    if (layout.viewportSnapButton.contains(point)) {
        return EditorViewportAction::ToggleGizmoSnap;
    }
    return EditorViewportAction::None;
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

EditorCloseDialogAction editorCloseDialogActionAt(
    const EditorLayout& layout,
    EditorPoint point) {
    if (layout.closeSaveButton.contains(point)) {
        return EditorCloseDialogAction::SaveAndExit;
    }
    if (layout.closeDiscardButton.contains(point)) {
        return EditorCloseDialogAction::DiscardAndExit;
    }
    if (layout.closeCancelButton.contains(point)) {
        return EditorCloseDialogAction::Cancel;
    }
    return EditorCloseDialogAction::None;
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
