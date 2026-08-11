#include "scene_history.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
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

int resolveSceneHistorySelection(
    const SceneDocument& currentScene,
    const SceneDocument& restoredScene) {
    const int snapshotSelection = restoredScene.selectedObject;
    if (currentScene.objects.size() != restoredScene.objects.size()) {
        return snapshotSelection;
    }

    std::unordered_set<std::uint64_t> remainingRuntimeIds;
    remainingRuntimeIds.reserve(restoredScene.objects.size());
    for (const SceneObject& object : restoredScene.objects) {
        if (object.runtimeId == 0 ||
            !remainingRuntimeIds.insert(object.runtimeId).second) {
            return snapshotSelection;
        }
    }
    for (const SceneObject& object : currentScene.objects) {
        if (object.runtimeId == 0 || remainingRuntimeIds.erase(object.runtimeId) != 1) {
            return snapshotSelection;
        }
    }
    if (!remainingRuntimeIds.empty() || currentScene.selectedObject < 0 ||
        currentScene.selectedObject >= static_cast<int>(currentScene.objects.size())) {
        return snapshotSelection;
    }

    return findSceneObjectByRuntimeId(
        restoredScene,
        currentScene.objects[static_cast<std::size_t>(currentScene.selectedObject)]
            .runtimeId);
}

void preserveSceneAnimationState(
    const SceneDocument& currentScene,
    SceneDocument& restoredScene) {
    std::unordered_map<std::uint64_t, const SceneObject*> currentObjects;
    currentObjects.reserve(currentScene.objects.size());
    for (const SceneObject& object : currentScene.objects) {
        if (object.runtimeId != 0) {
            currentObjects.emplace(object.runtimeId, &object);
        }
    }

    for (SceneObject& restoredObject : restoredScene.objects) {
        if (!restoredObject.spinning || restoredObject.runtimeId == 0) {
            continue;
        }
        const auto current = currentObjects.find(restoredObject.runtimeId);
        if (current != currentObjects.end() && current->second->spinning) {
            restoredObject.rotationDeg = current->second->rotationDeg;
        }
    }

    if (currentScene.pointLight.spinning && restoredScene.pointLight.spinning) {
        restoredScene.pointLight.position.x = currentScene.pointLight.position.x;
        restoredScene.pointLight.position.z = currentScene.pointLight.position.z;
    }
}

SceneHistory::SceneHistory(std::size_t maxEntries, std::size_t maxBytes)
    : m_maxEntries(maxEntries), m_maxBytes(maxBytes) {
    const std::size_t reserveCount = std::min<std::size_t>(maxEntries, 64);
    m_undo.reserve(reserveCount);
    m_redo.reserve(reserveCount);
}

void SceneHistory::record(SceneDocument scene, bool preserveAnimationState) {
    const std::size_t sceneBytes = estimateBytes(scene);
    if (m_maxEntries == 0 || m_maxBytes == 0 || sceneBytes > m_maxBytes) {
        return;
    }

    m_redo.clear();
    m_redoBytes = 0;
    pushBounded(
        m_undo, m_undoBytes, std::move(scene), preserveAnimationState);
}

const SceneDocument* SceneHistory::undoTarget() const {
    return m_undo.empty() ? nullptr : &m_undo.back().scene;
}

const SceneDocument* SceneHistory::redoTarget() const {
    return m_redo.empty() ? nullptr : &m_redo.back().scene;
}

bool SceneHistory::undoPreservesAnimationState() const {
    return !m_undo.empty() && m_undo.back().preserveAnimationState;
}

bool SceneHistory::redoPreservesAnimationState() const {
    return !m_redo.empty() && m_redo.back().preserveAnimationState;
}

bool SceneHistory::commitUndo(SceneDocument currentScene) {
    if (m_undo.empty()) {
        return false;
    }

    Entry target = std::move(m_undo.back());
    m_undoBytes -= target.bytes;
    m_undo.pop_back();
    if (!pushBounded(
            m_redo, m_redoBytes, std::move(currentScene),
            target.preserveAnimationState)) {
        m_undoBytes += target.bytes;
        m_undo.push_back(std::move(target));
        return false;
    }
    return true;
}

bool SceneHistory::commitRedo(SceneDocument currentScene) {
    if (m_redo.empty()) {
        return false;
    }

    Entry target = std::move(m_redo.back());
    m_redoBytes -= target.bytes;
    m_redo.pop_back();
    if (!pushBounded(
            m_undo, m_undoBytes, std::move(currentScene),
            target.preserveAnimationState)) {
        m_redoBytes += target.bytes;
        m_redo.push_back(std::move(target));
        return false;
    }
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

bool SceneHistory::pushBounded(
    std::vector<Entry>& stack, std::size_t& usedBytes, SceneDocument scene,
    bool preserveAnimationState) {
    if (m_maxEntries == 0 || m_maxBytes == 0) {
        return false;
    }

    const std::size_t sceneBytes = estimateBytes(scene);
    if (sceneBytes > m_maxBytes) {
        return false;
    }

    const std::size_t otherStackBytes =
        &stack == &m_undo ? m_redoBytes : m_undoBytes;
    if (otherStackBytes > m_maxBytes ||
        sceneBytes > m_maxBytes - otherStackBytes) {
        return false;
    }
    const std::size_t maximumUsedBytes =
        m_maxBytes - otherStackBytes - sceneBytes;
    while (!stack.empty() &&
           (stack.size() >= m_maxEntries || usedBytes > maximumUsedBytes)) {
        usedBytes -= stack.front().bytes;
        stack.erase(stack.begin());
    }
    if (usedBytes > maximumUsedBytes) {
        return false;
    }
    stack.push_back({std::move(scene), sceneBytes, preserveAnimationState});
    usedBytes += sceneBytes;
    return true;
}

} // namespace core
