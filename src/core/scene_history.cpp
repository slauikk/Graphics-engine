#include "scene_history.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace {

void addBytes(std::size_t& total, std::size_t value) {
    const std::size_t maximum = (std::numeric_limits<std::size_t>::max)();
    total = value > maximum - total ? maximum : total + value;
}

void addElements(std::size_t& total, std::size_t count, std::size_t elementSize) {
    const std::size_t maximum = (std::numeric_limits<std::size_t>::max)();
    addBytes(total, count > maximum / elementSize ? maximum : count * elementSize);
}

} // namespace

namespace core {

SceneHistory::SceneHistory(std::size_t maxEntries, std::size_t maxBytes)
    : m_maxEntries(maxEntries), m_maxBytes(maxBytes) {
    const std::size_t reserveCount = std::min<std::size_t>(maxEntries, 64);
    m_undo.reserve(reserveCount);
    m_redo.reserve(reserveCount);
}

void SceneHistory::record(SceneDocument scene) {
    m_redo.clear();
    m_redoBytes = 0;
    pushBounded(m_undo, m_undoBytes, std::move(scene));
}

const SceneDocument* SceneHistory::undoTarget() const {
    return m_undo.empty() ? nullptr : &m_undo.back().scene;
}

const SceneDocument* SceneHistory::redoTarget() const {
    return m_redo.empty() ? nullptr : &m_redo.back().scene;
}

bool SceneHistory::commitUndo(SceneDocument currentScene) {
    if (m_undo.empty()) {
        return false;
    }
    m_undoBytes -= m_undo.back().bytes;
    m_undo.pop_back();
    pushBounded(m_redo, m_redoBytes, std::move(currentScene));
    return true;
}

bool SceneHistory::commitRedo(SceneDocument currentScene) {
    if (m_redo.empty()) {
        return false;
    }
    m_redoBytes -= m_redo.back().bytes;
    m_redo.pop_back();
    pushBounded(m_undo, m_undoBytes, std::move(currentScene));
    return true;
}

void SceneHistory::clear() {
    m_undo.clear();
    m_redo.clear();
    m_undoBytes = 0;
    m_redoBytes = 0;
}

std::size_t SceneHistory::estimateBytes(const SceneDocument& scene) {
    std::size_t bytes = sizeof(SceneDocument);
    addElements(bytes, scene.materials.size(), sizeof(SceneMaterial));
    addElements(bytes, scene.objects.size(), sizeof(SceneObject));
    for (const SceneMaterial& material : scene.materials) {
        addBytes(bytes, material.id.size());
        addBytes(bytes, material.albedoTexture.size());
    }
    for (const SceneObject& object : scene.objects) {
        addBytes(bytes, object.name.size());
        addBytes(bytes, object.mesh.size());
        addBytes(bytes, object.material.size());
    }
    return bytes;
}

void SceneHistory::pushBounded(
    std::vector<Entry>& stack, std::size_t& usedBytes, SceneDocument scene) {
    if (m_maxEntries == 0 || m_maxBytes == 0) {
        return;
    }

    const std::size_t sceneBytes = estimateBytes(scene);
    if (sceneBytes > m_maxBytes) {
        return;
    }

    const auto hasByteCapacity = [this, sceneBytes] {
        if (m_redoBytes > m_maxBytes ||
            m_undoBytes > m_maxBytes - m_redoBytes) {
            return false;
        }
        return m_undoBytes + m_redoBytes <= m_maxBytes - sceneBytes;
    };
    while (!stack.empty() &&
           (stack.size() >= m_maxEntries || !hasByteCapacity())) {
        usedBytes -= stack.front().bytes;
        stack.erase(stack.begin());
    }
    if (!hasByteCapacity()) {
        return;
    }
    stack.push_back({std::move(scene), sceneBytes});
    usedBytes += sceneBytes;
}

} // namespace core
