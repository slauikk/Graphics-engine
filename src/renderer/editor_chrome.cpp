#include "editor_chrome.h"

#include "../core/resource_manager.h"
#include "../shader.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <iterator>

namespace {

struct Color {
    float red;
    float green;
    float blue;
};

constexpr Color kMenuBarColor{0.043f, 0.047f, 0.055f};
constexpr Color kToolbarColor{0.078f, 0.086f, 0.102f};
constexpr Color kPanelColor{0.071f, 0.078f, 0.090f};
constexpr Color kPanelHeaderColor{0.102f, 0.110f, 0.125f};
constexpr Color kViewportHeaderColor{0.086f, 0.094f, 0.110f};
constexpr Color kStatusColor{0.051f, 0.055f, 0.063f};
constexpr Color kButtonColor{0.129f, 0.141f, 0.161f};
constexpr Color kButtonHoverColor{0.188f, 0.208f, 0.235f};
constexpr Color kButtonActiveColor{0.722f, 0.435f, 0.125f};
constexpr Color kButtonBorderColor{0.220f, 0.239f, 0.271f};
constexpr Color kHierarchyHoverColor{0.125f, 0.137f, 0.157f};
constexpr Color kSelectionColor{0.204f, 0.165f, 0.106f};
constexpr Color kSeparatorColor{0.153f, 0.165f, 0.188f};
constexpr Color kViewportBorderColor{0.208f, 0.227f, 0.255f};
constexpr Color kSplitterHoverColor{0.302f, 0.573f, 0.831f};
constexpr Color kModalShadowColor{0.020f, 0.022f, 0.027f};
constexpr Color kModalColor{0.071f, 0.078f, 0.090f};
constexpr Color kPopupColor{0.086f, 0.094f, 0.106f};
constexpr Color kDisabledColor{0.090f, 0.098f, 0.110f};
constexpr std::array<Color, 3> kGizmoAxisColors = {
    Color{0.92f, 0.24f, 0.20f},
    Color{0.24f, 0.82f, 0.36f},
    Color{0.22f, 0.48f, 0.96f}};
constexpr Color kGizmoHoverColor{1.0f, 0.82f, 0.26f};
constexpr Color kGizmoOriginColor{0.92f, 0.94f, 0.98f};

class ScopedChromeRenderState {
public:
    ScopedChromeRenderState()
        : m_depthTestEnabled(glIsEnabled(GL_DEPTH_TEST)),
          m_blendEnabled(glIsEnabled(GL_BLEND)) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &m_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &m_vertexArray);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &m_arrayBuffer);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    ~ScopedChromeRenderState() {
        glUseProgram(static_cast<GLuint>(m_program));
        glBindVertexArray(static_cast<GLuint>(m_vertexArray));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_arrayBuffer));
        if (m_depthTestEnabled == GL_TRUE) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        if (m_blendEnabled == GL_TRUE) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
    }

private:
    GLboolean m_depthTestEnabled = GL_FALSE;
    GLboolean m_blendEnabled = GL_FALSE;
    GLint m_program = 0;
    GLint m_vertexArray = 0;
    GLint m_arrayBuffer = 0;
};

} // namespace

EditorChrome::~EditorChrome() {
    if (m_vertexBuffer != 0) {
        glDeleteBuffers(1, &m_vertexBuffer);
    }
    if (m_vertexArray != 0) {
        glDeleteVertexArrays(1, &m_vertexArray);
    }
}

bool EditorChrome::init() {
    m_shader = ResourceManager::getShader(
        "shaders/ui_rect.vert", "shaders/ui_rect.frag");
    if (!m_shader || m_shader->m_id == 0) {
        return false;
    }

    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    if (m_vertexArray == 0 || m_vertexBuffer == 0) {
        return false;
    }

    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    constexpr GLsizei stride = 5 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_vertices.reserve(32 * 6 * 5);
    return true;
}

void EditorChrome::appendRect(
    const core::EditorRect& rect,
    float red,
    float green,
    float blue) {
    if (!rect.valid()) {
        return;
    }

    const float left = static_cast<float>(rect.x);
    const float top = static_cast<float>(rect.y);
    const float right = static_cast<float>(rect.x + rect.width);
    const float bottom = static_cast<float>(rect.y + rect.height);
    const float vertices[] = {
        left,  top,    red, green, blue,
        right, top,    red, green, blue,
        right, bottom, red, green, blue,
        left,  top,    red, green, blue,
        right, bottom, red, green, blue,
        left,  bottom, red, green, blue};
    m_vertices.insert(
        m_vertices.end(), std::begin(vertices), std::end(vertices));
}

void EditorChrome::appendOutline(
    const core::EditorRect& rect,
    float red,
    float green,
    float blue) {
    if (!rect.valid()) {
        return;
    }
    appendRect({rect.x, rect.y, rect.width, 1}, red, green, blue);
    appendRect(
        {rect.x, rect.y + rect.height - 1, rect.width, 1},
        red, green, blue);
    appendRect({rect.x, rect.y, 1, rect.height}, red, green, blue);
    appendRect(
        {rect.x + rect.width - 1, rect.y, 1, rect.height},
        red, green, blue);
}

void EditorChrome::appendLine(
    core::EditorPoint start,
    core::EditorPoint end,
    float thickness,
    float red,
    float green,
    float blue) {
    const double deltaX = end.x - start.x;
    const double deltaY = end.y - start.y;
    const double length = std::hypot(deltaX, deltaY);
    if (!std::isfinite(length) || length <= 0.0 ||
        !std::isfinite(thickness) || thickness <= 0.0f) {
        return;
    }

    const float halfThickness = thickness * 0.5f;
    const float normalX =
        static_cast<float>(-deltaY / length) * halfThickness;
    const float normalY =
        static_cast<float>(deltaX / length) * halfThickness;
    const float startX = static_cast<float>(start.x);
    const float startY = static_cast<float>(start.y);
    const float endX = static_cast<float>(end.x);
    const float endY = static_cast<float>(end.y);
    const float vertices[] = {
        startX + normalX, startY + normalY, red, green, blue,
        endX + normalX,   endY + normalY,   red, green, blue,
        endX - normalX,   endY - normalY,   red, green, blue,
        startX + normalX, startY + normalY, red, green, blue,
        endX - normalX,   endY - normalY,   red, green, blue,
        startX - normalX, startY - normalY, red, green, blue};
    m_vertices.insert(
        m_vertices.end(), std::begin(vertices), std::end(vertices));
}

void EditorChrome::render(
    const core::EditorLayout& layout,
    int selectedObject,
    std::size_t firstVisibleObject,
    std::size_t objectCount,
    bool canDuplicateObject,
    bool canDeleteObject,
    bool gridEnabled,
    bool menuOpen,
    bool closeDialogOpen,
    core::EditorMenu openEditorMenu,
    core::EditorPoint cursor,
    const core::EditorTranslationGizmo& gizmo,
    core::EditorGizmoAxis activeGizmoAxis,
    core::EditorPanelSplitter activePanelSplitter) {
    if (layout.width <= 0 || layout.height <= 0 ||
        !m_shader || m_shader->m_id == 0 ||
        m_vertexArray == 0 || m_vertexBuffer == 0) {
        return;
    }

    m_vertices.clear();
    const auto appendColorRect = [this](
        const core::EditorRect& rect, Color color) {
        appendRect(rect, color.red, color.green, color.blue);
    };
    const auto appendColorOutline = [this](
        const core::EditorRect& rect, Color color) {
        appendOutline(rect, color.red, color.green, color.blue);
    };
    const bool editorMenuOpen = openEditorMenu != core::EditorMenu::None;
    const bool interactionBlocked =
        menuOpen || closeDialogOpen || editorMenuOpen;

    appendColorRect(layout.menuBar, kMenuBarColor);
    appendColorRect(layout.toolbar, kToolbarColor);
    appendColorRect(layout.hierarchy, kPanelColor);
    appendColorRect(layout.hierarchyHeader, kPanelHeaderColor);
    appendColorRect(layout.viewportHeader, kViewportHeaderColor);
    appendColorRect(layout.inspector, kPanelColor);
    appendColorRect(layout.inspectorHeader, kPanelHeaderColor);
    appendColorRect(layout.statusBar, kStatusColor);

    const auto appendButton = [&](
        const core::EditorRect& button, bool active, bool interactive = true) {
        const bool hovered = interactive && button.contains(cursor);
        appendColorRect(
            button,
            active ? kButtonActiveColor
                   : (hovered ? kButtonHoverColor : kButtonColor));
        appendColorOutline(button, kButtonBorderColor);
    };
    appendButton(layout.createButton, false, !interactionBlocked);
    appendButton(layout.saveButton, false, !interactionBlocked);
    appendButton(layout.loadButton, false, !interactionBlocked);
    appendButton(layout.gridButton, gridEnabled, !interactionBlocked);
    appendButton(layout.assetsButton, menuOpen, !closeDialogOpen);
    appendButton(layout.benchmarkButton, false, !interactionBlocked);
    appendButton(layout.hierarchyToggleButton, false, !interactionBlocked);
    appendButton(layout.inspectorToggleButton, false, !interactionBlocked);

    constexpr std::array menus = {
        core::EditorMenu::File,
        core::EditorMenu::Edit,
        core::EditorMenu::View,
        core::EditorMenu::Window};
    for (std::size_t index = 0; index < menus.size(); ++index) {
        const core::EditorRect& button = layout.menuButtons[index];
        const bool active = openEditorMenu == menus[index];
        const bool hovered = !closeDialogOpen && button.contains(cursor);
        if (active || hovered) {
            appendColorRect(
                button,
                active ? kPanelHeaderColor : kToolbarColor);
        }
        if (active) {
            appendColorRect(
                {button.x, button.y + button.height - 2,
                 button.width, 2},
                kButtonActiveColor);
        }
    }

    const auto appendToolbarSeparator = [&](const core::EditorRect& after) {
        if (!after.valid()) {
            return;
        }
        appendColorRect(
            {after.x + after.width + 3,
             layout.toolbar.y + 10,
             1,
             (std::max)(0, layout.toolbar.height - 20)},
            kSeparatorColor);
    };
    appendToolbarSeparator(layout.createButton);
    appendToolbarSeparator(layout.loadButton);
    appendToolbarSeparator(layout.assetsButton);

    const auto appendHierarchyButton = [&](
        const core::EditorRect& button, bool enabled) {
        const bool hovered = enabled && !interactionBlocked &&
            button.contains(cursor);
        appendColorRect(
            button,
            enabled
                ? (hovered ? kButtonHoverColor : kButtonColor)
                : kPanelHeaderColor);
        appendColorOutline(button, kSeparatorColor);
    };
    appendHierarchyButton(
        layout.hierarchyDuplicateButton, canDuplicateObject);
    appendHierarchyButton(
        layout.hierarchyDeleteButton, canDeleteObject);

    const bool inspectorEnabled = selectedObject >= 0 &&
        static_cast<std::size_t>(selectedObject) < objectCount;
    const auto appendInspectorButton = [&](const core::EditorRect& button) {
        const bool hovered = inspectorEnabled && !interactionBlocked &&
            button.contains(cursor);
        appendColorRect(
            button,
            inspectorEnabled
                ? (hovered ? kButtonHoverColor : kButtonColor)
                : kPanelHeaderColor);
        appendColorOutline(button, kSeparatorColor);
    };
    for (const core::EditorRect& button : layout.inspectorMoveButtons) {
        appendInspectorButton(button);
    }
    for (const core::EditorRect& button : layout.inspectorRotateScaleButtons) {
        appendInspectorButton(button);
    }
    for (const core::EditorRect& button : layout.inspectorSnapResetButtons) {
        appendInspectorButton(button);
    }

    if (!interactionBlocked && layout.hierarchyList.contains(cursor)) {
        const int relativeY =
            static_cast<int>(cursor.y) - layout.hierarchyList.y;
        const std::size_t visibleRow = static_cast<std::size_t>(
            relativeY / core::kEditorHierarchyRowHeight);
        if (firstVisibleObject + visibleRow < objectCount) {
            appendColorRect(
                core::editorHierarchyRowRect(layout, visibleRow),
                kHierarchyHoverColor);
        }
    }

    if (selectedObject >= 0 &&
        static_cast<std::size_t>(selectedObject) < objectCount &&
        static_cast<std::size_t>(selectedObject) >= firstVisibleObject) {
        const std::size_t visibleRow =
            static_cast<std::size_t>(selectedObject) - firstVisibleObject;
        if (visibleRow < core::editorHierarchyVisibleRowCount(layout)) {
            const core::EditorRect row =
                core::editorHierarchyRowRect(layout, visibleRow);
            appendColorRect(row, kSelectionColor);
            appendColorRect({row.x, row.y, 3, row.height}, kButtonActiveColor);
        }
    }

    if (gizmo.valid && !interactionBlocked) {
        constexpr std::array<core::EditorGizmoAxis, 3> axes = {
            core::EditorGizmoAxis::X,
            core::EditorGizmoAxis::Y,
            core::EditorGizmoAxis::Z};
        const core::EditorGizmoAxis hoveredAxis =
            core::editorGizmoAxisAt(gizmo, cursor);
        for (std::size_t index = 0; index < axes.size(); ++index) {
            if (!gizmo.handles[index].valid()) {
                continue;
            }
            const bool highlighted = axes[index] == activeGizmoAxis ||
                axes[index] == hoveredAxis;
            const Color color = highlighted
                ? kGizmoHoverColor
                : kGizmoAxisColors[index];
            appendLine(
                gizmo.origin, gizmo.endpoints[index],
                highlighted ? 4.0f : 3.0f,
                color.red, color.green, color.blue);
            appendColorRect(gizmo.handles[index], color);
            appendColorOutline(gizmo.handles[index], kModalShadowColor);
        }
        appendColorRect(
            {static_cast<int>(std::lround(gizmo.origin.x)) - 4,
             static_cast<int>(std::lround(gizmo.origin.y)) - 4,
             8, 8},
            kGizmoOriginColor);
    }

    appendColorOutline(layout.viewportFrame, kViewportBorderColor);
    appendColorRect(
        {layout.viewportHeader.x,
         layout.viewportHeader.y + layout.viewportHeader.height - 1,
         layout.viewportHeader.width,
         1},
        kSeparatorColor);
    appendColorRect(
        {0, layout.menuBar.y + layout.menuBar.height - 1,
         layout.width, 1},
        kSeparatorColor);
    appendColorRect(
        {0, layout.toolbar.y + layout.toolbar.height - 1,
         layout.width, 1},
        kSeparatorColor);
    appendColorRect(
        {layout.hierarchy.x + layout.hierarchy.width,
         layout.hierarchy.y, 1, layout.hierarchy.height},
        kSeparatorColor);
    appendColorRect(
        {layout.inspector.x - 1,
         layout.inspector.y, 1, layout.inspector.height},
        kSeparatorColor);
    appendColorRect(
        {0, layout.statusBar.y - 1, layout.width, 1},
        kSeparatorColor);

    const core::EditorPanelSplitter hoveredSplitter = interactionBlocked
        ? core::EditorPanelSplitter::None
        : core::editorPanelSplitterAt(layout, cursor);
    const auto appendSplitterHighlight = [&](const core::EditorRect& splitter,
                                              core::EditorPanelSplitter panel) {
        if (!splitter.valid() ||
            (hoveredSplitter != panel && activePanelSplitter != panel)) {
            return;
        }
        const int highlightWidth = (std::min)(3, splitter.width);
        const core::EditorRect highlight = {
            splitter.x + (splitter.width - highlightWidth) / 2,
            splitter.y,
            highlightWidth,
            splitter.height};
        appendColorRect(
            highlight,
            activePanelSplitter == panel
                ? kButtonActiveColor
                : kSplitterHoverColor);
    };
    appendSplitterHighlight(
        layout.hierarchySplitter, core::EditorPanelSplitter::Hierarchy);
    appendSplitterHighlight(
        layout.inspectorSplitter, core::EditorPanelSplitter::Inspector);

    if ((menuOpen || closeDialogOpen) && layout.modalOverlay.valid()) {
        appendColorRect(
            {layout.modalOverlay.x + 8, layout.modalOverlay.y + 8,
             layout.modalOverlay.width, layout.modalOverlay.height},
            kModalShadowColor);
        appendColorRect(layout.modalOverlay, kModalColor);
        appendColorOutline(layout.modalOverlay, kViewportBorderColor);
    }
    if (closeDialogOpen) {
        appendButton(layout.closeSaveButton, false, true);
        appendButton(layout.closeDiscardButton, false, true);
        appendButton(layout.closeCancelButton, false, true);
    }

    drawVertices(layout);
}

void EditorChrome::renderMenuPopup(
    const core::EditorLayout& layout,
    core::EditorMenu openEditorMenu,
    const std::array<bool, core::kEditorMenuMaximumItems>&
        editorMenuItemsEnabled,
    core::EditorPoint cursor) {
    if (openEditorMenu == core::EditorMenu::None ||
        layout.width <= 0 || layout.height <= 0 ||
        !m_shader || m_shader->m_id == 0 ||
        m_vertexArray == 0 || m_vertexBuffer == 0) {
        return;
    }

    m_vertices.clear();
    const core::EditorMenuPopup popup =
        core::calculateEditorMenuPopup(layout, openEditorMenu);
    if (!popup.bounds.valid()) {
        return;
    }
    appendRect(
        {popup.bounds.x + 6, popup.bounds.y + 6,
         popup.bounds.width, popup.bounds.height},
        kModalShadowColor.red,
        kModalShadowColor.green,
        kModalShadowColor.blue);
    appendRect(
        popup.bounds, kPopupColor.red, kPopupColor.green, kPopupColor.blue);
    appendOutline(
        popup.bounds,
        kButtonBorderColor.red,
        kButtonBorderColor.green,
        kButtonBorderColor.blue);
    for (std::size_t index = 0; index < popup.itemCount; ++index) {
        const bool enabled = editorMenuItemsEnabled[index];
        const bool hovered = enabled && popup.items[index].contains(cursor);
        if (hovered) {
            appendRect(
                popup.items[index],
                kButtonHoverColor.red,
                kButtonHoverColor.green,
                kButtonHoverColor.blue);
        } else if (!enabled) {
            appendRect(
                popup.items[index],
                kDisabledColor.red,
                kDisabledColor.green,
                kDisabledColor.blue);
        }
    }
    drawVertices(layout);
}

void EditorChrome::drawVertices(const core::EditorLayout& layout) {
    if (m_vertices.empty()) {
        return;
    }

    const ScopedChromeRenderState restoreState;
    m_shader->use();
    const glm::mat4 projection = glm::ortho(
        0.0f, static_cast<float>(layout.width),
        static_cast<float>(layout.height), 0.0f);
    m_shader->setMat4("projection", projection);

    glBindVertexArray(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(float)),
        m_vertices.data(),
        GL_STREAM_DRAW);
    glDrawArrays(
        GL_TRIANGLES, 0,
        static_cast<GLsizei>(m_vertices.size() / 5));
}
