#ifndef SCENE_HISTORY_H
#define SCENE_HISTORY_H

#include "scene_document.h"

#include <cstddef>
#include <vector>

namespace core {

inline constexpr std::size_t kDefaultSceneHistoryBytes = 64U * 1024U * 1024U;

int resolveSceneHistorySelection(
    const SceneDocument& currentScene,
    const SceneDocument& restoredScene);

void preserveSceneAnimationState(
    const SceneDocument& currentScene,
    SceneDocument& restoredScene);

class SceneHistory {
public:
    explicit SceneHistory(
        std::size_t maxEntries = 64,
        std::size_t maxBytes = kDefaultSceneHistoryBytes);

    void record(SceneDocument scene, bool preserveAnimationState = true);
    const SceneDocument* undoTarget() const;
    const SceneDocument* redoTarget() const;
    bool undoPreservesAnimationState() const;
    bool redoPreservesAnimationState() const;
    bool commitUndo(SceneDocument currentScene);
    bool commitRedo(SceneDocument currentScene);
    void clear();

    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    std::size_t undoDepth() const { return m_undo.size(); }
    std::size_t redoDepth() const { return m_redo.size(); }

private:
    struct Entry {
        SceneDocument scene;
        std::size_t bytes = 0;
        bool preserveAnimationState = true;
    };

    static std::size_t estimateBytes(const SceneDocument& scene);
    void pushBounded(std::vector<Entry>& stack, std::size_t& usedBytes,
                     SceneDocument scene, bool preserveAnimationState);

    std::size_t m_maxEntries;
    std::size_t m_maxBytes;
    std::size_t m_undoBytes = 0;
    std::size_t m_redoBytes = 0;
    std::vector<Entry> m_undo;
    std::vector<Entry> m_redo;
};

} // namespace core

#endif // SCENE_HISTORY_H
