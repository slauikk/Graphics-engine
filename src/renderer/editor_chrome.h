#ifndef EDITOR_CHROME_H
#define EDITOR_CHROME_H

#include "../core/editor_layout.h"
#include "../core/editor_gizmo.h"

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
        bool menuOpen,
        bool closeDialogOpen,
        core::EditorPoint cursor,
        const core::EditorTranslationGizmo& gizmo,
        core::EditorGizmoAxis activeGizmoAxis,
        core::EditorPanelSplitter activePanelSplitter);

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

    unsigned int m_vertexArray = 0;
    unsigned int m_vertexBuffer = 0;
    std::shared_ptr<Shader> m_shader;
    std::vector<float> m_vertices;
};

#endif // EDITOR_CHROME_H
