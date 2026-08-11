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

constexpr Color kToolbarColor{0.047f, 0.059f, 0.078f};
constexpr Color kPanelColor{0.067f, 0.082f, 0.106f};
constexpr Color kPanelHeaderColor{0.086f, 0.106f, 0.137f};
constexpr Color kStatusColor{0.039f, 0.047f, 0.063f};
constexpr Color kButtonColor{0.114f, 0.137f, 0.176f};
constexpr Color kButtonHoverColor{0.173f, 0.216f, 0.282f};
constexpr Color kButtonActiveColor{0.255f, 0.176f, 0.071f};
constexpr Color kHierarchyHoverColor{0.125f, 0.153f, 0.196f};
constexpr Color kSelectionColor{0.235f, 0.153f, 0.047f};
constexpr Color kSeparatorColor{0.184f, 0.220f, 0.275f};
constexpr Color kViewportBorderColor{0.714f, 0.463f, 0.110f};
constexpr Color kModalShadowColor{0.020f, 0.024f, 0.031f};
constexpr Color kModalColor{0.055f, 0.067f, 0.086f};
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
    core::EditorPoint cursor,
    const core::EditorTranslationGizmo& gizmo,
    core::EditorGizmoAxis activeGizmoAxis) {
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

    appendColorRect(layout.toolbar, kToolbarColor);
    appendColorRect(layout.hierarchy, kPanelColor);
    appendColorRect(layout.hierarchyHeader, kPanelHeaderColor);
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
        appendColorOutline(button, kSeparatorColor);
    };
    appendButton(layout.createButton, false, !menuOpen);
    appendButton(layout.saveButton, false, !menuOpen);
    appendButton(layout.loadButton, false, !menuOpen);
    appendButton(layout.gridButton, gridEnabled, !menuOpen);
    appendButton(layout.assetsButton, menuOpen, true);
    appendButton(layout.benchmarkButton, false, !menuOpen);
    appendButton(layout.hierarchyToggleButton, false, !menuOpen);
    appendButton(layout.inspectorToggleButton, false, !menuOpen);

    const auto appendHierarchyButton = [&](
        const core::EditorRect& button, bool enabled) {
        const bool hovered = enabled && !menuOpen && button.contains(cursor);
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
        const bool hovered = inspectorEnabled && !menuOpen &&
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

    if (!menuOpen && layout.hierarchyList.contains(cursor)) {
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
        static_cast<std::size_t>(selectedObject) >= firstVisibleObject) {
        const std::size_t visibleRow =
            static_cast<std::size_t>(selectedObject) - firstVisibleObject;
        appendColorRect(
            core::editorHierarchyRowRect(layout, visibleRow),
            kSelectionColor);
    }

    if (gizmo.valid && !menuOpen) {
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

    appendColorOutline(layout.viewport, kViewportBorderColor);
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

    if (menuOpen && layout.modalOverlay.valid()) {
        appendColorRect(
            {layout.modalOverlay.x + 8, layout.modalOverlay.y + 8,
             layout.modalOverlay.width, layout.modalOverlay.height},
            kModalShadowColor);
        appendColorRect(layout.modalOverlay, kModalColor);
        appendColorOutline(layout.modalOverlay, kViewportBorderColor);
    }

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
