#ifndef EDITOR_CHROME_H
#define EDITOR_CHROME_H

#include "../core/editor_layout.h"
#include "../core/editor_gizmo.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

class Shader;

class EditorChrome {
public:
    EditorChrome() = default;
    ~EditorChrome();

    EditorChrome(const EditorChrome&) = delete;
    EditorChrome& operator=(const EditorChrome&) = delete;

    bool init();
    void render(
        const core::EditorLayout& layout,
        int selectedObject,
        std::size_t firstVisibleObject,
        std::size_t objectCount,
        bool canDuplicateObject,
        bool canDeleteObject,
        bool gridEnabled,
        bool debugViewEnabled,
        bool postProcessEnabled,
        core::EditorGizmoMode gizmoMode,
        bool gizmoSnapEnabled,
        bool menuOpen,
        bool closeDialogOpen,
        core::EditorMenu openEditorMenu,
        core::EditorPoint cursor,
        const core::EditorTranslationGizmo& translationGizmo,
        const core::EditorRotationGizmo& rotationGizmo,
        core::EditorGizmoAxis activeGizmoAxis,
        core::EditorPanelSplitter activePanelSplitter);
    void renderMenuPopup(
        const core::EditorLayout& layout,
        core::EditorMenu openEditorMenu,
        const std::array<bool, core::kEditorMenuMaximumItems>&
            editorMenuItemsEnabled,
        core::EditorPoint cursor);

private:
    void appendRect(const core::EditorRect& rect, float red, float green, float blue);
    void appendOutline(const core::EditorRect& rect, float red, float green, float blue);
    void appendLine(
        core::EditorPoint start,
        core::EditorPoint end,
        float thickness,
        float red,
        float green,
        float blue);
    void drawVertices(const core::EditorLayout& layout);

    unsigned int m_vertexArray = 0;
    unsigned int m_vertexBuffer = 0;
    std::shared_ptr<Shader> m_shader;
    std::vector<float> m_vertices;
};

#endif // EDITOR_CHROME_H
