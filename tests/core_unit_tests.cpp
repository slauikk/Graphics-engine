#include "core/asset_paths.h"
#include "core/benchmark_report.h"
#include "core/editor_camera.h"
#include "core/editor_gizmo.h"
#include "core/editor_layout.h"
#include "core/editor_placement.h"
#include "core/editor_transform.h"
#include "core/scene_document.h"
#include "core/scene_history.h"
#include "core/spatial_query.h"
#include "core/window_settings.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool almostEqual(double left, double right) {
    const double scale = std::max({1.0, std::abs(left), std::abs(right)});
    return std::abs(left - right) <= 0.000001 * scale;
}

std::array<glm::vec3, 8> boundsCorners(
    const geometry::AxisAlignedBounds& bounds) {
    return {
        glm::vec3(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z),
        glm::vec3(bounds.maximum.x, bounds.minimum.y, bounds.minimum.z),
        glm::vec3(bounds.minimum.x, bounds.maximum.y, bounds.minimum.z),
        glm::vec3(bounds.maximum.x, bounds.maximum.y, bounds.minimum.z),
        glm::vec3(bounds.minimum.x, bounds.minimum.y, bounds.maximum.z),
        glm::vec3(bounds.maximum.x, bounds.minimum.y, bounds.maximum.z),
        glm::vec3(bounds.minimum.x, bounds.maximum.y, bounds.maximum.z),
        glm::vec3(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z)};
}

void requireBoundsInsideFrustum(
    const geometry::AxisAlignedBounds& bounds,
    const glm::vec3& cameraPosition,
    const glm::vec3& viewDirection,
    float verticalFovDegrees,
    float aspectRatio,
    float nearPlane,
    float farPlane,
    float margin,
    const std::string& message) {
    const glm::vec3 direction = glm::normalize(viewDirection);
    glm::vec3 right = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::dot(right, right) <= 1.0e-12f) {
        right = glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    right = glm::normalize(right);
    const glm::vec3 up = glm::normalize(glm::cross(right, direction));
    const glm::mat4 view = glm::lookAt(
        cameraPosition, cameraPosition + direction, up);
    const glm::mat4 projection = glm::perspective(
        glm::radians(verticalFovDegrees), aspectRatio, nearPlane, farPlane);
    const float lateralLimit = 1.0f / margin + 0.0001f;
    for (const glm::vec3& corner : boundsCorners(bounds)) {
        const glm::vec4 clip = projection * view * glm::vec4(corner, 1.0f);
        require(clip.w > 0.0f, message + ": corner is behind the camera");
        const glm::vec3 normalized = glm::vec3(clip) / clip.w;
        require(std::abs(normalized.x) <= lateralLimit &&
                    std::abs(normalized.y) <= lateralLimit &&
                    normalized.z >= -1.0001f && normalized.z <= 1.0001f,
                message + ": corner is outside the clip volume");
    }
}

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        m_path = std::filesystem::temp_directory_path() /
                 ("graphics_engine_core_tests_" + std::to_string(suffix));
        std::filesystem::create_directories(m_path);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    const std::filesystem::path& path() const {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

core::BenchmarkReport validBenchmarkReport() {
    core::BenchmarkReport report;
    report.workload = "core unit test";
    report.gpuVendor = "Test Vendor";
    report.gpuRenderer = "Test Renderer";
    report.openGlVersion = "4.6 test";
    report.targetWidth = 1600;
    report.targetHeight = 900;
    report.displayWidth = 1920;
    report.displayHeight = 1080;
    report.instances = 2'304;
    report.shaderIterations = 48;
    report.warmupSeconds = 2.0;
    report.draw = core::makeBenchmarkMetric({1.0f, 2.0f, 3.0f});
    report.frameInterval = core::makeBenchmarkMetric({4.0f, 5.0f, 6.0f});
    report.cpuWork = core::makeBenchmarkMetric({2.0f, 2.5f, 3.0f});
    report.present = core::makeBenchmarkMetric({0.5f, 0.75f, 1.0f});
    report.gpuTotal = core::makeBenchmarkMetric({3.0f, 3.5f, 4.0f});
    report.gpuScene = core::makeBenchmarkMetric({2.0f, 2.25f, 2.5f});
    report.gpuUi = core::makeBenchmarkMetric({0.1f, 0.2f, 0.3f});
    return report;
}

core::SceneDocument validScene() {
    core::SceneDocument scene;
    core::SceneMaterial material;
    material.id = "material:test";
    material.albedoTexture = "textures/jager.png";
    scene.materials.push_back(material);

    core::SceneObject object;
    object.name = "Test cube";
    object.mesh = "builtin:cube";
    object.material = material.id;
    scene.objects.push_back(object);
    scene.selectedObject = 0;
    return scene;
}

void testAssetPathContainment() {
    TemporaryDirectory temporary;
    const std::filesystem::path assetsRoot = temporary.path() / "assets";
    const std::filesystem::path models = assetsRoot / "models";
    const std::filesystem::path textures = assetsRoot / "textures";
    const std::filesystem::path outside = temporary.path() / "outside";
    const std::filesystem::path lookalike = temporary.path() / "assets_backup";
    std::filesystem::create_directories(models);
    std::filesystem::create_directories(textures);
    std::filesystem::create_directories(outside);
    std::filesystem::create_directories(lookalike);

    const std::filesystem::path material = models / "sample.mtl";
    const std::filesystem::path texture = textures / "albedo.png";
    const std::filesystem::path secret = outside / "secret.bin";
    const std::filesystem::path lookalikeFile = lookalike / "decoy.bin";
    std::ofstream(material) << "material";
    std::ofstream(texture) << "texture";
    std::ofstream(secret) << "secret";
    std::ofstream(lookalikeFile) << "decoy";

    const auto localDependency = core::resolveContainedPath(
        assetsRoot, models, "sample.mtl");
    require(localDependency.has_value() &&
                std::filesystem::equivalent(*localDependency, material),
            "same-directory asset dependency was rejected");

    const auto siblingDependency = core::resolveContainedPath(
        assetsRoot, models, "../textures/albedo.png");
    require(siblingDependency.has_value() &&
                std::filesystem::equivalent(*siblingDependency, texture),
            "contained sibling asset dependency was rejected");

    require(core::resolveContainedPath(assetsRoot, models, texture).has_value(),
            "absolute path inside the assets root was rejected");
    require(!core::resolveContainedPath(
                 assetsRoot, models, "../../outside/secret.bin").has_value(),
            "parent traversal escaped the assets root");
    require(!core::resolveContainedPath(assetsRoot, models, secret).has_value(),
            "absolute path outside the assets root was accepted");
    require(!core::resolveContainedPath(
                 assetsRoot, models, lookalikeFile).has_value(),
            "lookalike assets directory prefix bypassed containment");
    require(!core::resolveContainedPath(
                 assetsRoot, models, std::filesystem::path()).has_value(),
            "empty dependency path was accepted");
#ifdef _WIN32
    require(!core::resolveContainedPath(
                 assetsRoot, models, std::filesystem::path(L"\\outside\\secret.bin"))
                 .has_value(),
            "root-relative Windows dependency path was accepted");
#endif
}

void testEditorLayoutAndHitTesting() {
    const core::EditorLayout layout =
        core::calculateEditorLayout(1280, 720);
    require(layout.toolbar.x == 0 && layout.toolbar.y == 0 &&
                layout.toolbar.width == 1280 && layout.toolbar.height == 44,
            "editor toolbar bounds are incorrect");
    require(layout.statusBar.y == 692 && layout.statusBar.height == 28,
            "editor status bar bounds are incorrect");
    require(layout.hierarchy.width == 230 &&
                layout.inspector.x == 986 && layout.inspector.width == 294,
            "editor side panel bounds are incorrect");
    require(layout.viewport.x == 231 && layout.viewport.y == 44 &&
                layout.viewport.width == 754 && layout.viewport.height == 648,
            "editor viewport bounds are incorrect");
    require(layout.modalOverlay.x == 348 && layout.modalOverlay.y == 118 &&
                layout.modalOverlay.width == 520 &&
                layout.modalOverlay.height == 500,
            "editor modal overlay bounds are incorrect");
    require(layout.viewport.contains({500.0, 300.0}) &&
                !layout.viewport.contains({100.0, 300.0}) &&
                !layout.viewport.contains({1100.0, 300.0}),
            "editor viewport hit testing crossed panel boundaries");

    const core::EditorPoint framebufferPoint =
        core::mapWindowPointToFramebuffer(
            {320.0, 180.0}, 640, 360, 1280, 720);
    require(framebufferPoint.x == 640.0 && framebufferPoint.y == 360.0,
            "high-DPI cursor mapping is incorrect");
    const core::EditorPoint invalidPoint =
        core::mapWindowPointToFramebuffer(
            {10.0, 10.0}, 0, 360, 1280, 720);
    require(invalidPoint.x < 0.0 && invalidPoint.y < 0.0,
            "invalid cursor dimensions produced a usable point");

    require(core::editorToolbarActionAt(
                layout,
                {layout.createButton.x + 2.0, layout.createButton.y + 2.0}) ==
                core::EditorToolbarAction::CreateObject &&
                core::editorToolbarActionAt(
                    layout,
                    {layout.gridButton.x + 2.0, layout.gridButton.y + 2.0}) ==
                core::EditorToolbarAction::ToggleGrid &&
                core::editorToolbarActionAt(layout, {20.0, 20.0}) ==
                core::EditorToolbarAction::None,
            "editor toolbar action hit testing is incorrect");

    require(core::editorHierarchyActionAt(
                layout,
                {layout.hierarchyDuplicateButton.x + 2.0,
                 layout.hierarchyDuplicateButton.y + 2.0}) ==
                core::EditorHierarchyAction::DuplicateObject &&
                core::editorHierarchyActionAt(
                    layout,
                    {layout.hierarchyDeleteButton.x + 2.0,
                     layout.hierarchyDeleteButton.y + 2.0}) ==
                core::EditorHierarchyAction::DeleteObject &&
                core::editorHierarchyActionAt(
                    layout,
                    {layout.hierarchyList.x + 2.0,
                     layout.hierarchyList.y + 2.0}) ==
                core::EditorHierarchyAction::None,
            "editor hierarchy action hit testing is incorrect");

    require(core::editorPanelActionAt(
                layout,
                {layout.hierarchyToggleButton.x + 2.0,
                 layout.hierarchyToggleButton.y + 2.0}) ==
                core::EditorPanelAction::ToggleHierarchy &&
                core::editorPanelActionAt(
                    layout,
                    {layout.inspectorToggleButton.x + 2.0,
                     layout.inspectorToggleButton.y + 2.0}) ==
                core::EditorPanelAction::ToggleInspector &&
                core::editorPanelActionAt(
                    layout,
                    {layout.viewport.x + 2.0,
                     layout.viewport.y + 2.0}) ==
                core::EditorPanelAction::None,
            "editor panel toggle hit testing is incorrect");

    const core::EditorRect moveNegativeX =
        layout.inspectorMoveButtons[0];
    const core::EditorRect reset =
        layout.inspectorSnapResetButtons[1];
    require(core::editorInspectorTransformAt(
                layout,
                {moveNegativeX.x + moveNegativeX.width * 0.5,
                 moveNegativeX.y + moveNegativeX.height * 0.5}) ==
                core::ObjectTransformCommand::MoveNegativeX &&
                core::editorInspectorTransformAt(
                    layout,
                    {reset.x + reset.width * 0.5,
                     reset.y + reset.height * 0.5}) ==
                core::ObjectTransformCommand::Reset &&
                !core::editorInspectorTransformAt(
                    layout, {layout.viewport.x + 4.0,
                             layout.viewport.y + 4.0}).has_value(),
            "editor inspector transform hit testing is incorrect");

    const std::size_t visibleRows =
        core::editorHierarchyVisibleRowCount(layout);
    require(visibleRows == 22,
            "editor hierarchy visible row count is incorrect");
    const std::size_t firstVisible =
        core::firstVisibleEditorObject(layout, 30, 29);
    require(firstVisible == 8,
            "editor hierarchy did not keep the selected object visible");
    const core::EditorRect lastRow =
        core::editorHierarchyRowRect(layout, visibleRows - 1);
    const auto selectedObject = core::editorHierarchyObjectAt(
        layout,
        {lastRow.x + 4.0, lastRow.y + 4.0},
        30, firstVisible);
    require(selectedObject.has_value() && *selectedObject == 29,
            "editor hierarchy row hit the wrong scene object");

    const core::EditorLayout compact =
        core::calculateEditorLayout(500, 300);
    require(compact.viewport.width >= 320 && compact.viewport.height > 0,
            "compact editor layout collapsed the viewport");

    const core::EditorLayout collapsed =
        core::calculateEditorLayout(1280, 720, false, false);
    require(!collapsed.hierarchyExpanded && !collapsed.inspectorExpanded &&
                collapsed.hierarchy.width ==
                    core::kEditorCollapsedPanelWidth &&
                collapsed.inspector.width ==
                    core::kEditorCollapsedPanelWidth &&
                collapsed.viewport.x == 35 &&
                collapsed.viewport.width == 1210,
            "collapsed panels did not expand the editor viewport");
    require(!collapsed.hierarchyList.valid() &&
                !collapsed.hierarchyDuplicateButton.valid() &&
                !collapsed.inspectorContent.valid() &&
                collapsed.hierarchyToggleButton.valid() &&
                collapsed.inspectorToggleButton.valid(),
            "collapsed panel controls have invalid visibility");
}

void testEditorTranslationGizmo() {
    const core::EditorRect viewport{100, 50, 800, 600};
    const glm::mat4 view = glm::lookAt(
        glm::vec3(4.0f, 3.0f, 6.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    const core::EditorTranslationGizmo gizmo =
        core::calculateEditorTranslationGizmo(
            viewport, glm::vec3(0.0f), view, projection);
    require(gizmo.valid && viewport.contains(gizmo.origin),
            "visible object did not produce a translation gizmo");

    constexpr std::array<core::EditorGizmoAxis, 3> axes = {
        core::EditorGizmoAxis::X,
        core::EditorGizmoAxis::Y,
        core::EditorGizmoAxis::Z};
    for (std::size_t index = 0; index < axes.size(); ++index) {
        require(gizmo.handles[index].valid() &&
                    gizmo.worldUnitsPerPixel[index] > 0.0f,
                "projected gizmo axis is invalid");
        const core::EditorRect& handle = gizmo.handles[index];
        require(core::editorGizmoAxisAt(
                    gizmo,
                    {handle.x + handle.width * 0.5,
                     handle.y + handle.height * 0.5}) == axes[index],
                "gizmo handle selected the wrong axis");
    }
    const core::EditorRect& xHandle = gizmo.handles[0];
    require(core::editorGizmoAxisAt(
                gizmo,
                {xHandle.x - core::kEditorGizmoHitPadding + 1.0,
                 xHandle.y + xHandle.height * 0.5}) ==
                core::EditorGizmoAxis::X,
            "gizmo high-DPI hit padding is not interactive");
    core::EditorTranslationGizmo overlapping = gizmo;
    overlapping.handles[0] = {100, 100, 12, 12};
    overlapping.handles[1] = {110, 100, 12, 12};
    overlapping.handles[2] = {};
    require(core::editorGizmoAxisAt(overlapping, {114.0, 106.0}) ==
                core::EditorGizmoAxis::Y,
            "overlapping gizmo handles did not choose the nearest axis");
    require(core::editorGizmoAxisAt(
                gizmo, {viewport.x + 2.0, viewport.y + 2.0}) ==
                core::EditorGizmoAxis::None,
            "empty viewport space selected a gizmo axis");

    const glm::vec2 xDirection = gizmo.screenDirections[0];
    const core::EditorPoint draggedCursor{
        gizmo.origin.x + static_cast<double>(xDirection.x * 40.0f),
        gizmo.origin.y + static_cast<double>(xDirection.y * 40.0f)};
    const auto translated = core::calculateEditorGizmoTranslation(
        gizmo,
        core::EditorGizmoAxis::X,
        glm::vec3(1.0f, 2.0f, 3.0f),
        gizmo.origin,
        draggedCursor);
    require(translated.has_value() && translated->x > 1.0f &&
                almostEqual(translated->y, 2.0) &&
                almostEqual(translated->z, 3.0),
            "gizmo drag did not translate only the selected axis");
    const auto snapped = core::calculateEditorGizmoTranslation(
        gizmo,
        core::EditorGizmoAxis::X,
        glm::vec3(0.13f, 2.2f, 3.3f),
        gizmo.origin,
        draggedCursor,
        core::kObjectTranslationStep);
    require(snapped.has_value() &&
                almostEqual(
                    snapped->x / core::kObjectTranslationStep,
                    std::round(snapped->x / core::kObjectTranslationStep)) &&
                almostEqual(snapped->y, 2.2) &&
                almostEqual(snapped->z, 3.3),
            "gizmo snapping changed an inactive axis or missed the grid");
    require(!core::calculateEditorGizmoTranslation(
                 gizmo,
                 core::EditorGizmoAxis::X,
                 glm::vec3(0.0f),
                 gizmo.origin,
                 draggedCursor,
                 -core::kObjectTranslationStep)
                 .has_value(),
            "gizmo drag accepted a negative snap step");
    require(!core::calculateEditorGizmoTranslation(
                 gizmo,
                 core::EditorGizmoAxis::X,
                 glm::vec3(0.0f),
                 gizmo.origin,
                 draggedCursor,
                 std::numeric_limits<float>::infinity())
                 .has_value(),
            "gizmo drag accepted a non-finite snap step");
    require(!core::calculateEditorGizmoTranslation(
                 gizmo,
                 core::EditorGizmoAxis::None,
                 glm::vec3(0.0f),
                 gizmo.origin,
                 draggedCursor)
                 .has_value(),
            "gizmo drag accepted the None axis");
    require(!core::editorGizmoTranslationChanged(
                glm::vec3(1.0f, 2.0f, 3.0f),
                glm::vec3(1.0f, 2.0f, 3.0f)) &&
                core::editorGizmoTranslationChanged(
                    glm::vec3(1.0f, 2.0f, 3.0f),
                    glm::vec3(1.5f, 2.0f, 3.0f)) &&
                !core::editorGizmoTranslationChanged(
                    glm::vec3(0.0f),
                    glm::vec3(
                        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f)),
            "gizmo transaction change detection accepted an invalid no-op");

    const core::EditorTranslationGizmo hidden =
        core::calculateEditorTranslationGizmo(
            viewport,
            glm::vec3(0.0f, 0.0f, 20.0f),
            view,
            projection);
    require(!hidden.valid,
            "object behind the camera produced an interactive gizmo");
    const core::EditorTranslationGizmo beyondFarPlane =
        core::calculateEditorTranslationGizmo(
            viewport,
            glm::vec3(0.0f, 0.0f, -500.0f),
            view,
            projection);
    require(!beyondFarPlane.valid,
            "object beyond the far plane produced an interactive gizmo");
}

void testWindowSettingsPersistence() {
    core::WindowSettings defaults;
    std::string error;
    require(core::validateWindowSettings(defaults, error),
            "default window settings are invalid: " + error);

    const core::WindowWorkArea workArea{100, 50, 1920, 1040};
    const core::WindowSettings centered =
        core::fitWindowSettingsToWorkArea(defaults, workArea);
    require(centered.hasPosition && centered.x == 420 && centered.y == 210 &&
                centered.width == 1280 && centered.height == 720,
            "default window was not centered in the monitor work area");

    core::WindowSettings offscreen = defaults;
    offscreen.hasPosition = true;
    offscreen.x = 999'999;
    offscreen.y = -999'999;
    offscreen.width = 1600;
    offscreen.height = 900;
    const core::WindowSettings clamped =
        core::fitWindowSettingsToWorkArea(offscreen, workArea);
    require(clamped.x == 420 && clamped.y == 50 &&
                clamped.width == 1600 && clamped.height == 900,
            "off-screen window bounds were not clamped to the work area");

    const core::WindowSettings fittedSmall =
        core::fitWindowSettingsToWorkArea(
            defaults, core::WindowWorkArea{0, 0, 1024, 650});
    require(fittedSmall.x == 0 && fittedSmall.y == 0 &&
                fittedSmall.width == 1024 && fittedSmall.height == 650,
            "window dimensions were not fitted to a smaller work area");

    const core::WindowSettings fittedTiny =
        core::fitWindowSettingsToWorkArea(
            defaults, core::WindowWorkArea{-640, 0, 640, 480});
    require(fittedTiny.x == -640 && fittedTiny.y == 0 &&
                fittedTiny.width == 640 && fittedTiny.height == 480 &&
                core::validateWindowSettings(fittedTiny, error),
            "sub-minimum work area produced settings that cannot be persisted");
    const core::WindowSettings expandedTiny =
        core::fitWindowSettingsToWorkArea(
            fittedTiny, core::WindowWorkArea{0, 0, 1024, 768});
    require(expandedTiny.width == core::kMinimumEditorWindowWidth &&
                expandedTiny.height == core::kMinimumEditorWindowHeight,
            "stored small window was not restored to the editor minimum");

    const std::vector<core::WindowWorkArea> workAreas = {
        {-1920, 0, 1920, 1080},
        {0, 0, 2560, 1400}};
    core::WindowSettings laptopWindow = defaults;
    laptopWindow.hasPosition = true;
    laptopWindow.x = -1760;
    laptopWindow.y = 80;
    require(core::windowWorkAreaIndexForSettings(
                laptopWindow, workAreas, 1) == 0,
            "window on the secondary monitor was forced to the primary monitor");
    core::WindowSettings primaryWindow = defaults;
    primaryWindow.hasPosition = true;
    primaryWindow.x = 640;
    primaryWindow.y = 336;
    require(core::windowWorkAreaIndexForSettings(
                primaryWindow, workAreas, 0) == 1,
            "window on the primary monitor selected the wrong work area");
    require(core::windowWorkAreaIndexForSettings(
                defaults, workAreas, 1) == 1,
            "unpositioned window ignored the primary-monitor fallback");
    core::WindowSettings disconnectedWindow = defaults;
    disconnectedWindow.hasPosition = true;
    disconnectedWindow.x = -20'000;
    require(core::windowWorkAreaIndexForSettings(
                disconnectedWindow, workAreas, 1) == 1,
            "disconnected monitor position did not fall back to primary");

    TemporaryDirectory temporary;
    const std::filesystem::path settingsPath =
        temporary.path() / "editor_settings.json";
    const core::WindowSettingsLoadResult missing =
        core::loadWindowSettings(settingsPath);
    require(!missing.loaded && missing.error.empty() &&
                missing.settings.width == core::kDefaultEditorWindowWidth,
            "missing window settings did not return clean defaults");

    const std::filesystem::path tinySettingsPath =
        temporary.path() / "tiny_editor_settings.json";
    const core::WindowSettingsSaveResult tinySaved =
        core::saveWindowSettings(fittedTiny, tinySettingsPath);
    const core::WindowSettingsLoadResult tinyLoaded =
        core::loadWindowSettings(tinySettingsPath);
    require(tinySaved.success && tinyLoaded.loaded &&
                tinyLoaded.settings.width == 640 &&
                tinyLoaded.settings.height == 480,
            "small work-area settings did not survive persistence");

    core::WindowSettings source = clamped;
    source.fullscreen = true;
    source.vsync = false;
    source.hierarchyExpanded = false;
    source.inspectorExpanded = false;
    const core::WindowSettingsSaveResult saved =
        core::saveWindowSettings(source, settingsPath);
    require(saved.success, "window settings save failed: " + saved.error);
    const core::WindowSettingsLoadResult loaded =
        core::loadWindowSettings(settingsPath);
    require(loaded.loaded && loaded.error.empty() &&
                loaded.settings.x == source.x &&
                loaded.settings.y == source.y &&
                loaded.settings.width == source.width &&
                loaded.settings.height == source.height &&
                loaded.settings.hasPosition == source.hasPosition &&
                loaded.settings.fullscreen == source.fullscreen &&
                loaded.settings.vsync == source.vsync &&
                loaded.settings.hierarchyExpanded ==
                    source.hierarchyExpanded &&
                loaded.settings.inspectorExpanded ==
                    source.inspectorExpanded,
            "window settings round-trip changed persisted values");

    {
        std::ofstream legacy(settingsPath, std::ios::binary | std::ios::trunc);
        legacy << R"({
  "schema_version": 1,
  "x": -1759,
  "y": 87,
  "width": 1280,
  "height": 720,
  "has_position": true,
  "fullscreen": false,
  "vsync": true
})";
    }
    const core::WindowSettingsLoadResult migrated =
        core::loadWindowSettings(settingsPath);
    require(migrated.loaded && migrated.error.empty() &&
                migrated.settings.schemaVersion ==
                    core::kCurrentWindowSettingsSchemaVersion &&
                migrated.settings.x == -1759 &&
                migrated.settings.hierarchyExpanded &&
                migrated.settings.inspectorExpanded,
            "schema-1 window settings were not migrated safely");

    {
        std::ofstream malformed(settingsPath, std::ios::binary | std::ios::trunc);
        malformed << "{ not valid json";
    }
    const core::WindowSettingsLoadResult malformed =
        core::loadWindowSettings(settingsPath);
    require(!malformed.loaded && !malformed.error.empty() &&
                malformed.settings.width == core::kDefaultEditorWindowWidth,
            "malformed window settings were accepted");

    core::WindowSettings invalid = defaults;
    invalid.width = 0;
    const core::WindowSettingsSaveResult invalidSave =
        core::saveWindowSettings(invalid, settingsPath);
    require(!invalidSave.success && !invalidSave.error.empty(),
            "invalid window settings were saved");
}

void testBenchmarkStatistics() {
    const std::vector<float> empty;
    const core::BenchmarkStatistics emptyStatistics =
        core::calculateBenchmarkStatistics(empty);
    require(emptyStatistics.medianMs == 0.0, "empty median must be zero");

    const std::vector<float> odd = {3.0f, 1.0f, 2.0f};
    const core::BenchmarkStatistics oddStatistics =
        core::calculateBenchmarkStatistics(odd);
    require(almostEqual(oddStatistics.medianMs, 2.0), "odd median is incorrect");
    require(almostEqual(oddStatistics.p95Ms, 3.0), "odd p95 is incorrect");
    require(almostEqual(oddStatistics.meanMs, 2.0), "odd mean is incorrect");
    require(almostEqual(oddStatistics.minMs, 1.0), "minimum is incorrect");
    require(almostEqual(oddStatistics.maxMs, 3.0), "maximum is incorrect");
    require(odd == std::vector<float>({3.0f, 1.0f, 2.0f}),
            "statistics calculation changed the input order");

    const core::BenchmarkStatistics evenStatistics =
        core::calculateBenchmarkStatistics({4.0f, 1.0f, 3.0f, 2.0f});
    require(almostEqual(evenStatistics.medianMs, 2.5), "even median is incorrect");
    require(almostEqual(evenStatistics.p95Ms, 4.0), "even p95 is incorrect");

    core::BenchmarkMetric metric = core::makeBenchmarkMetric({3.0f, 1.0f, 2.0f});
    require(metric.samplesMs == odd, "makeBenchmarkMetric changed sample order");
    require(almostEqual(metric.statistics.medianMs, 2.0),
            "makeBenchmarkMetric did not calculate statistics");
}

void testBenchmarkRoundTripAndMultilineCsv() {
    TemporaryDirectory temporary;
    core::BenchmarkReport first = validBenchmarkReport();
    first.workload = "Quoted, multiline\nworkload";
    first.gpuRenderer = "Renderer \"A\"\nLine 2";

    const core::BenchmarkReportResult firstResult =
        core::writeBenchmarkReport(first, temporary.path());
    require(firstResult.success, "first benchmark report failed: " + firstResult.error);
    require(std::filesystem::is_regular_file(firstResult.jsonPath),
            "benchmark JSON was not created");
    require(std::filesystem::is_regular_file(firstResult.csvPath),
            "benchmark CSV was not created");

    std::ifstream jsonInput(firstResult.jsonPath, std::ios::binary);
    const nlohmann::json document = nlohmann::json::parse(jsonInput);
    require(document.at("schema_version") == 2, "benchmark schema version is incorrect");
    require(document.at("gpu").at("renderer") == first.gpuRenderer,
            "JSON renderer did not round-trip");
    require(document.at("metrics_ms").at("draw").at("samples").size() == 3,
            "JSON samples are missing");

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    core::BenchmarkReport second = first;
    second.gpuRenderer = "Renderer B";
    const core::BenchmarkReportResult secondResult =
        core::writeBenchmarkReport(second, temporary.path());
    require(secondResult.success, "second benchmark report failed: " + secondResult.error);

    const auto comparison =
        core::findLatestCompatibleGpuComparison(second, secondResult);
    require(comparison.has_value(), "multiline CSV comparison was not found");
    require(comparison->gpuRenderer == first.gpuRenderer,
            "multiline CSV renderer did not round-trip");
    require(almostEqual(comparison->draw.medianMs, first.draw.statistics.medianMs),
            "comparison median is incorrect");
}

void testBenchmarkRejectsInvalidReports() {
    TemporaryDirectory temporary;

    core::BenchmarkReport invalidWarmup = validBenchmarkReport();
    invalidWarmup.warmupSeconds = std::numeric_limits<double>::quiet_NaN();
    require(!core::writeBenchmarkReport(invalidWarmup, temporary.path() / "nan").success,
            "NaN warmup was accepted");

    core::BenchmarkReport negativeWarmup = validBenchmarkReport();
    negativeWarmup.warmupSeconds = -1.0;
    require(!core::writeBenchmarkReport(negativeWarmup, temporary.path() / "negative").success,
            "negative warmup was accepted");

    core::BenchmarkReport forgedStatistics = validBenchmarkReport();
    forgedStatistics.draw.statistics.medianMs = -500.0;
    require(!core::writeBenchmarkReport(
                forgedStatistics, temporary.path() / "forged").success,
            "statistics inconsistent with samples were accepted");

    core::BenchmarkReport mismatchedSamples = validBenchmarkReport();
    mismatchedSamples.gpuUi = core::makeBenchmarkMetric({0.1f, 0.2f});
    require(!core::writeBenchmarkReport(
                mismatchedSamples, temporary.path() / "mismatch").success,
            "mismatched sample counts were accepted");

    core::BenchmarkReport invalidSample = validBenchmarkReport();
    invalidSample.present.samplesMs[1] = -0.5f;
    require(!core::writeBenchmarkReport(
                invalidSample, temporary.path() / "sample").success,
            "negative sample was accepted");
}

void testBenchmarkRejectsInvalidComparison() {
    TemporaryDirectory temporary;
    const std::filesystem::path csvPath = temporary.path() / "benchmark_summary_v2.csv";
    {
        std::ofstream csv(csvPath, std::ios::binary | std::ios::trunc);
        csv << "timestamp_utc,gpu_vendor,gpu_renderer,opengl_version,workload,"
               "target_width,target_height,display_width,display_height,instances,"
               "shader_iterations,warmup_seconds,sample_count,draw_median_ms,"
               "draw_p95_ms,frame_interval_median_ms,cpu_work_median_ms,"
               "present_median_ms,gpu_total_median_ms,gpu_scene_median_ms,"
               "gpu_ui_median_ms,json_file\n"
               "2026-01-01T00:00:00Z,Other Vendor,Other Renderer,4.6 test,"
               "core unit test,1600,900,1920,1080,2304,48,2.0,3,2.0,-1.0,"
               "5.0,2.5,0.75,3.5,2.25,0.2,previous.json\n";
    }

    core::BenchmarkReportResult currentResult;
    currentResult.success = true;
    currentResult.timestampUtc = "2026-01-02T00:00:00Z";
    currentResult.csvPath = csvPath;
    require(!core::findLatestCompatibleGpuComparison(
                 validBenchmarkReport(), currentResult).has_value(),
            "negative comparison percentile was accepted");

    {
        std::ofstream csv(csvPath, std::ios::binary | std::ios::trunc);
        csv << "timestamp_utc,gpu_vendor,gpu_renderer,opengl_version,workload,"
               "target_width,target_height,display_width,display_height,instances,"
               "shader_iterations,warmup_seconds,sample_count,draw_median_ms,"
               "draw_p95_ms,frame_interval_median_ms,cpu_work_median_ms,"
               "present_median_ms,gpu_total_median_ms,gpu_scene_median_ms,"
               "gpu_ui_median_ms,json_file\n"
               "2026-01-01T00:00:00Z,Other Vendor,Other Renderer,4.6 test,"
               "core unit test,1600,900,2560,1440,2304,48,2.0,3,2.0,3.0,"
               "5.0,2.5,0.75,3.5,2.25,0.2,previous.json\n";
    }
    require(!core::findLatestCompatibleGpuComparison(
                 validBenchmarkReport(), currentResult).has_value(),
            "benchmark comparison accepted a different display resolution");
}

void testSceneValidationMatrix() {
    std::string error;
    core::SceneDocument scene = validScene();
    require(core::validateSceneDocument(scene, error), "valid scene was rejected: " + error);

    core::SceneDocument imported = scene;
    imported.objects[0].mesh = "models/sample_pyramid.obj";
    imported.objects[0].material.clear();
    require(core::validateSceneDocument(imported, error),
            "imported model without override material was rejected: " + error);

    core::SceneDocument cubeWithoutMaterial = scene;
    cubeWithoutMaterial.objects[0].material.clear();
    require(!core::validateSceneDocument(cubeWithoutMaterial, error),
            "built-in cube without material was accepted");

    core::SceneDocument unsupportedBuiltIn = scene;
    unsupportedBuiltIn.objects[0].mesh = "builtin:sphere";
    require(!core::validateSceneDocument(unsupportedBuiltIn, error),
            "unsupported built-in mesh was accepted");

    core::SceneDocument duplicateMaterial = scene;
    duplicateMaterial.materials.push_back(duplicateMaterial.materials.front());
    require(!core::validateSceneDocument(duplicateMaterial, error),
            "duplicate material ID was accepted");

    core::SceneDocument traversal = imported;
    traversal.objects[0].mesh = "models/../outside.obj";
    require(!core::validateSceneDocument(traversal, error),
            "asset traversal was accepted");

    core::SceneDocument nonFinite = scene;
    nonFinite.objects[0].position.x = std::numeric_limits<float>::quiet_NaN();
    require(!core::validateSceneDocument(nonFinite, error),
            "non-finite transform was accepted");

    core::SceneDocument zeroScale = scene;
    zeroScale.objects[0].scale.x = 0.0f;
    require(!core::validateSceneDocument(zeroScale, error), "zero scale was accepted");

    core::SceneDocument invalidSelection = scene;
    invalidSelection.selectedObject = 4;
    require(!core::validateSceneDocument(invalidSelection, error),
            "out-of-range selection was accepted");
}

void testSceneIoRoundTrip() {
    TemporaryDirectory temporary;
    const std::filesystem::path scenePath = temporary.path() / "scene.json";

    core::SceneDocument source = validScene();
    source.camera.position = {1.0f, 2.0f, 3.0f};
    source.camera.fov = 67.0f;
    source.renderSettings.postProcessEffect = 3;
    source.renderSettings.shaderViewMode = 4;
    source.renderSettings.coordinateGrid = false;
    source.objects[0].rotationDeg = {10.0f, 20.0f, 30.0f};
    source.objects[0].runtimeId = 42;

    core::SceneIoResult saveResult = core::saveSceneDocument(source, scenePath);
    require(saveResult.success, "scene save failed: " + saveResult.error);

    core::SceneDocument loaded;
    core::SceneIoResult loadResult = core::loadSceneDocument(scenePath, loaded);
    require(loadResult.success, "scene load failed: " + loadResult.error);
    require(almostEqual(loaded.camera.fov, source.camera.fov), "camera FOV was not restored");
    require(loaded.renderSettings.postProcessEffect == 3,
            "post-process effect was not restored");
    require(loaded.renderSettings.shaderViewMode == 4,
            "shader view mode was not restored");
    require(!loaded.renderSettings.coordinateGrid, "coordinate grid state was not restored");
    require(loaded.objects[0].material == source.objects[0].material,
            "object material was not restored");
    require(loaded.objects[0].runtimeId == 0,
            "runtime object ID leaked into the scene file");

    source.camera.fov = 50.0f;
    saveResult = core::saveSceneDocument(source, scenePath);
    require(saveResult.success, "atomic scene overwrite failed: " + saveResult.error);
    loadResult = core::loadSceneDocument(scenePath, loaded);
    require(loadResult.success && almostEqual(loaded.camera.fov, 50.0),
            "overwritten scene was not loaded");

    const std::filesystem::path malformedPath = temporary.path() / "malformed.json";
    {
        std::ofstream malformed(malformedPath, std::ios::binary | std::ios::trunc);
        malformed << "{ not valid json";
    }
    core::SceneDocument unchanged = source;
    unchanged.camera.fov = 42.0f;
    loadResult = core::loadSceneDocument(malformedPath, unchanged);
    require(!loadResult.success, "malformed scene was accepted");
    require(almostEqual(unchanged.camera.fov, 42.0),
            "failed scene load modified the destination");
}

void testOversizedSceneIsNotSaved() {
    TemporaryDirectory temporary;
    core::SceneDocument scene;
    core::SceneObject object;
    object.mesh = "models/" + std::string(999, 'a') + ".obj";
    scene.objects.assign(30'000, object);
    scene.selectedObject = 0;

    const std::filesystem::path scenePath = temporary.path() / "oversized.scene.json";
    const core::SceneIoResult result = core::saveSceneDocument(scene, scenePath);
    require(!result.success, "scene larger than the loader limit was saved");
    require(result.error.find("32 MiB") != std::string::npos,
            "oversized scene returned the wrong error");
    require(!std::filesystem::exists(scenePath),
            "oversized scene left a destination file behind");
}

void testSceneHistoryTransactions() {
    core::SceneDocument sceneA = validScene();
    sceneA.objects[0].name = "A";
    core::SceneDocument sceneB = sceneA;
    sceneB.objects[0].name = "B";
    core::SceneDocument sceneC = sceneA;
    sceneC.objects[0].name = "C";

    core::SceneHistory history(2);
    history.record(sceneA);
    history.record(sceneB);
    require(history.undoDepth() == 2 && history.redoDepth() == 0,
            "history did not record snapshots");
    require(history.undoPreservesAnimationState(),
            "ordinary history entry disabled animation preservation");

    const core::SceneDocument* target = history.undoTarget();
    require(target != nullptr && target->objects[0].name == "B",
            "undo target is incorrect");
    require(history.undoDepth() == 2,
            "reading an undo target committed the transaction early");
    require(history.commitUndo(sceneC), "undo transaction did not commit");
    require(history.undoDepth() == 1 && history.redoDepth() == 1,
            "undo transaction updated the wrong stacks");

    target = history.undoTarget();
    require(target != nullptr && target->objects[0].name == "A",
            "second undo target is incorrect");
    require(history.commitUndo(sceneB), "second undo transaction did not commit");
    require(!history.canUndo() && history.redoDepth() == 2,
            "history did not reach its oldest state");

    target = history.redoTarget();
    require(target != nullptr && target->objects[0].name == "B",
            "redo target is incorrect");
    require(history.commitRedo(sceneA), "redo transaction did not commit");
    target = history.redoTarget();
    require(target != nullptr && target->objects[0].name == "C",
            "second redo target is incorrect");

    history.record(sceneB);
    require(!history.canRedo(), "new edit did not clear the redo branch");

    core::SceneHistory bounded(2);
    bounded.record(sceneA);
    bounded.record(sceneB);
    bounded.record(sceneC);
    require(bounded.undoDepth() == 2, "bounded history exceeded its capacity");
    target = bounded.undoTarget();
    require(target != nullptr && target->objects[0].name == "C",
            "bounded history lost the newest snapshot");
    require(bounded.commitUndo(sceneA), "bounded undo did not commit");
    target = bounded.undoTarget();
    require(target != nullptr && target->objects[0].name == "B",
            "bounded history did not evict the oldest snapshot");

    core::SceneHistory disabled(0);
    disabled.record(sceneA);
    require(!disabled.canUndo() && !disabled.commitUndo(sceneB),
            "zero-capacity history accepted a snapshot");

    core::SceneHistory byteLimited(64, 1);
    byteLimited.record(sceneA);
    require(!byteLimited.canUndo(),
            "history retained a snapshot larger than its byte budget");

    core::SceneDocument oversizedCurrent = sceneB;
    oversizedCurrent.objects[0].name.assign(8192, 'x');
    core::SceneHistory failedUndoHistory(64, 4096);
    failedUndoHistory.record(sceneA);
    require(failedUndoHistory.canUndo() &&
                !failedUndoHistory.commitUndo(oversizedCurrent) &&
                failedUndoHistory.undoDepth() == 1 &&
                failedUndoHistory.redoDepth() == 0,
            "failed undo transfer corrupted the history stacks");
    target = failedUndoHistory.undoTarget();
    require(target != nullptr && target->objects[0].name == "A",
            "failed undo transfer lost its original target");

    core::SceneHistory failedRedoHistory(64, 4096);
    failedRedoHistory.record(sceneA);
    require(failedRedoHistory.commitUndo(sceneB) &&
                !failedRedoHistory.commitRedo(oversizedCurrent) &&
                failedRedoHistory.undoDepth() == 0 &&
                failedRedoHistory.redoDepth() == 1,
            "failed redo transfer corrupted the history stacks");
    target = failedRedoHistory.redoTarget();
    require(target != nullptr && target->objects[0].name == "B",
            "failed redo transfer lost its original target");
    failedRedoHistory.record(oversizedCurrent);
    require(failedRedoHistory.redoDepth() == 1,
            "rejected history record cleared the redo branch");

    core::SceneHistory authoredStateHistory;
    authoredStateHistory.record(sceneA, false);
    require(!authoredStateHistory.undoPreservesAnimationState(),
            "authored scene restore enabled animation preservation");
    require(authoredStateHistory.commitUndo(sceneB) &&
                !authoredStateHistory.redoPreservesAnimationState(),
            "undo did not propagate authored-state history metadata");
    require(authoredStateHistory.commitRedo(sceneC) &&
                !authoredStateHistory.undoPreservesAnimationState(),
            "redo did not propagate authored-state history metadata");

    core::SceneDocument currentSelection = validScene();
    currentSelection.objects[0].runtimeId = 7;
    core::SceneObject currentSecond = currentSelection.objects[0];
    currentSecond.runtimeId = 11;
    currentSelection.objects.push_back(currentSecond);
    currentSelection.selectedObject = 0;

    core::SceneDocument reorderedSelection = currentSelection;
    std::swap(reorderedSelection.objects[0], reorderedSelection.objects[1]);
    reorderedSelection.selectedObject = 0;
    require(core::resolveSceneHistorySelection(
                currentSelection, reorderedSelection) == 1,
            "history did not preserve selection across a non-structural restore");

    core::SceneDocument structuralRestore = currentSelection;
    core::SceneObject createdObject = structuralRestore.objects[0];
    createdObject.runtimeId = 13;
    structuralRestore.objects.push_back(createdObject);
    structuralRestore.selectedObject = 2;
    require(core::resolveSceneHistorySelection(
                currentSelection, structuralRestore) == 2,
            "history overrode snapshot selection for a structural redo");

    core::SceneDocument deletedRestore = currentSelection;
    deletedRestore.objects.erase(deletedRestore.objects.begin());
    deletedRestore.selectedObject = 0;
    require(core::resolveSceneHistorySelection(
                currentSelection, deletedRestore) == 0,
            "history overrode snapshot selection for a structural undo");

    core::SceneDocument currentAnimation = validScene();
    currentAnimation.objects[0].runtimeId = 23;
    currentAnimation.objects[0].spinning = true;
    currentAnimation.objects[0].rotationDeg = {91.0f, 92.0f, 93.0f};
    currentAnimation.pointLight.spinning = true;
    currentAnimation.pointLight.position = {3.0f, 2.0f, -1.0f};

    core::SceneDocument restoredAnimation = currentAnimation;
    restoredAnimation.objects[0].position.x = -4.0f;
    restoredAnimation.objects[0].rotationDeg = {1.0f, 2.0f, 3.0f};
    restoredAnimation.pointLight.position = {-3.0f, 8.0f, 1.0f};
    core::preserveSceneAnimationState(currentAnimation, restoredAnimation);
    require(restoredAnimation.objects[0].rotationDeg ==
                currentAnimation.objects[0].rotationDeg &&
                restoredAnimation.objects[0].position.x == -4.0f,
            "history restore rewound an active object animation");
    require(restoredAnimation.pointLight.position ==
                glm::vec3(3.0f, 8.0f, -1.0f),
            "history restore did not isolate point-light orbit components");

    core::SceneDocument intentionalRotation = restoredAnimation;
    intentionalRotation.objects[0].spinning = false;
    intentionalRotation.objects[0].rotationDeg = {15.0f, 30.0f, 45.0f};
    intentionalRotation.pointLight.spinning = false;
    intentionalRotation.pointLight.position = {7.0f, 8.0f, 9.0f};
    core::preserveSceneAnimationState(currentAnimation, intentionalRotation);
    require(intentionalRotation.objects[0].rotationDeg ==
                glm::vec3(15.0f, 30.0f, 45.0f) &&
                intentionalRotation.pointLight.position ==
                    glm::vec3(7.0f, 8.0f, 9.0f),
            "history restore overrode an intentional animation state change");
}

void testRuntimeObjectLookup() {
    core::SceneDocument scene = validScene();
    scene.objects[0].runtimeId = 7;
    core::SceneObject second = scene.objects[0];
    second.runtimeId = 11;
    scene.objects.push_back(second);

    require(core::findSceneObjectByRuntimeId(scene, 7) == 0,
            "first runtime object ID was not found");
    require(core::findSceneObjectByRuntimeId(scene, 11) == 1,
            "second runtime object ID was not found");
    require(core::findSceneObjectByRuntimeId(scene, 0) == -1 &&
                core::findSceneObjectByRuntimeId(scene, 99) == -1,
            "missing runtime object ID returned an object");
}

void testSpatialQueries() {
    const glm::mat4 identityView(1.0f);
    const glm::mat4 squareProjection = glm::perspective(
        glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    const auto centerRay = core::calculateViewportRayDirection(
        50.0f, 50.0f, 100.0f, 100.0f,
        identityView, squareProjection);
    require(centerRay.has_value() &&
                almostEqual(centerRay->x, 0.0) &&
                almostEqual(centerRay->y, 0.0) &&
                almostEqual(centerRay->z, -1.0),
            "viewport center did not produce a forward ray");
    const auto upperRightRay = core::calculateViewportRayDirection(
        75.0f, 25.0f, 100.0f, 100.0f,
        identityView, squareProjection);
    require(upperRightRay.has_value() &&
                upperRightRay->x > 0.0f && upperRightRay->y > 0.0f &&
                upperRightRay->z < 0.0f,
            "viewport point did not produce the expected ray quadrant");
    const auto centerFarDistance = core::calculateRayDistanceToViewPlane(
        *centerRay, glm::vec3(0.0f, 0.0f, -1.0f), 100.0f);
    const auto cornerFarDistance = core::calculateRayDistanceToViewPlane(
        *upperRightRay, glm::vec3(0.0f, 0.0f, -1.0f), 100.0f);
    require(centerFarDistance.has_value() &&
                almostEqual(*centerFarDistance, 100.0) &&
                cornerFarDistance.has_value() && *cornerFarDistance > 100.0f,
            "view-plane depth was incorrectly treated as radial ray distance");
    require(!core::calculateRayDistanceToViewPlane(
                 glm::vec3(1.0f, 0.0f, 0.0f),
                 glm::vec3(0.0f, 0.0f, -1.0f), 100.0f)
                 .has_value() &&
                !core::calculateRayDistanceToViewPlane(
                    glm::vec3(0.0f),
                    glm::vec3(0.0f, 0.0f, -1.0f), 100.0f)
                    .has_value() &&
                !core::calculateRayDistanceToViewPlane(
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 0.0f, -1.0f), -1.0f)
                    .has_value(),
            "invalid view-plane ray inputs were accepted");
    require(!core::calculateViewportRayDirection(
                -1.0f, 50.0f, 100.0f, 100.0f,
                identityView, squareProjection).has_value() &&
                !core::calculateViewportRayDirection(
                    50.0f, 50.0f, 0.0f, 100.0f,
                    identityView, squareProjection).has_value() &&
                !core::calculateViewportRayDirection(
                    50.0f, 50.0f, 100.0f, 100.0f,
                    identityView, glm::mat4(0.0f)).has_value(),
            "invalid viewport ray inputs were accepted");

    geometry::AxisAlignedBounds bounds;
    bounds.minimum = glm::vec3(-1.0f);
    bounds.maximum = glm::vec3(1.0f);
    bounds.valid = true;

    const auto directHit = core::intersectRayAabb(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f), bounds);
    require(directHit.has_value() && almostEqual(*directHit, 4.0),
            "direct ray hit returned the wrong distance");

    const auto unnormalizedHit = core::intersectRayAabb(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -2.0f), bounds);
    require(unnormalizedHit.has_value() && almostEqual(*unnormalizedHit, 2.0),
            "ray parameter was changed by implicit direction normalization");

    const auto tinyDirectionHit = core::intersectRayAabb(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0e-9f), bounds);
    require(tinyDirectionHit.has_value() && almostEqual(*tinyDirectionHit, 4.0e9),
            "tiny nonzero ray direction was treated as parallel");

    require(!core::intersectRayAabb(
                glm::vec3(2.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f), bounds)
                 .has_value(),
            "parallel ray outside a slab was accepted");
    require(!core::intersectRayAabb(
                glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 1.0f), bounds)
                 .has_value(),
            "ray pointing away from the bounds was accepted");

    const auto insideHit = core::intersectRayAabb(
        glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), bounds);
    require(insideHit.has_value() && almostEqual(*insideHit, 0.0),
            "ray starting inside the bounds was rejected");

    geometry::AxisAlignedBounds unitBounds;
    unitBounds.minimum = glm::vec3(-0.5f);
    unitBounds.maximum = glm::vec3(0.5f);
    unitBounds.valid = true;
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::scale(transform, glm::vec3(2.0f, 1.0f, 4.0f));
    const auto transformedHit = core::intersectRayTransformedAabb(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
        unitBounds, transform);
    require(transformedHit.has_value() && almostEqual(*transformedHit, 3.0),
            "transformed bounds returned a local-space distance");

    glm::mat4 fartherTransform(1.0f);
    fartherTransform = glm::translate(fartherTransform, glm::vec3(0.0f, 0.0f, -2.0f));
    const auto fartherHit = core::intersectRayTransformedAabb(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
        unitBounds, fartherTransform);
    require(fartherHit.has_value() && *transformedHit < *fartherHit,
            "transformed hit distances cannot select the nearest object");

    const glm::mat4 singular = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 1.0f));
    require(!core::intersectRayTransformedAabb(
                glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                unitBounds, singular)
                 .has_value(),
            "singular object transform was accepted");

    const glm::mat4 projective = glm::perspective(
        glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    require(!core::intersectRayTransformedAabb(
                glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                unitBounds, projective)
                 .has_value(),
            "projective object transform was accepted");

    geometry::AxisAlignedBounds flatBounds;
    flatBounds.minimum = glm::vec3(-1.0f, -1.0f, 0.0f);
    flatBounds.maximum = glm::vec3(1.0f, 1.0f, 0.0f);
    flatBounds.valid = true;
    const auto flatHit = core::intersectRayAabb(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f), flatBounds);
    require(flatHit.has_value() && almostEqual(*flatHit, 5.0),
            "zero-volume bounds were rejected");

    const std::vector<float> indexedVertices = {
        -1.0f, -2.0f, 0.0f,
         3.0f,  1.0f, 2.0f,
         0.0f,  4.0f, 1.0f,
       500.0f, 500.0f, 500.0f
    };
    const std::vector<std::uint32_t> indexedTriangles = {0, 1, 2};
    const auto indexedBounds = core::calculateIndexedBounds(
        indexedVertices, 3, indexedTriangles);
    require(indexedBounds.has_value() && indexedBounds->maximum.x == 3.0f &&
                indexedBounds->maximum.y == 4.0f && indexedBounds->maximum.z == 2.0f,
            "unused outlier vertex changed indexed bounds");
    const std::vector<std::uint32_t> invalidBoundsIndices = {0, 1, 4};
    require(!core::calculateIndexedBounds(
                indexedVertices, 3, invalidBoundsIndices)
                 .has_value(),
            "out-of-range bounds index was accepted");

    geometry::AxisAlignedBounds secondBounds;
    secondBounds.minimum = glm::vec3(-4.0f, 0.0f, -3.0f);
    secondBounds.maximum = glm::vec3(-2.0f, 6.0f, 1.0f);
    secondBounds.valid = true;
    const std::vector<geometry::AxisAlignedBounds> boundsToMerge = {
        *indexedBounds, secondBounds
    };
    const auto mergedBounds = core::mergeBounds(boundsToMerge);
    require(mergedBounds.has_value() && mergedBounds->minimum.x == -4.0f &&
                mergedBounds->minimum.y == -2.0f && mergedBounds->maximum.x == 3.0f &&
                mergedBounds->maximum.y == 6.0f,
            "mesh bounds were not merged correctly");

    glm::mat4 boundsTransform(1.0f);
    boundsTransform = glm::translate(boundsTransform, glm::vec3(3.0f, 4.0f, 5.0f));
    boundsTransform = glm::scale(boundsTransform, glm::vec3(2.0f, 1.0f, 0.5f));
    const auto worldBounds = core::transformBounds(unitBounds, boundsTransform);
    require(worldBounds.has_value() && almostEqual(worldBounds->minimum.x, 2.0) &&
                almostEqual(worldBounds->maximum.x, 4.0) &&
                almostEqual(worldBounds->minimum.y, 3.5) &&
                almostEqual(worldBounds->maximum.y, 4.5) &&
                almostEqual(worldBounds->minimum.z, 4.75) &&
                almostEqual(worldBounds->maximum.z, 5.25),
            "affine bounds transform is incorrect");

    geometry::AxisAlignedBounds firstPart;
    firstPart.minimum = glm::vec3(-2.5f, -0.5f, -2.5f);
    firstPart.maximum = glm::vec3(-1.5f, 0.5f, -1.5f);
    firstPart.valid = true;
    geometry::AxisAlignedBounds secondPart;
    secondPart.minimum = glm::vec3(1.5f, -0.5f, 1.5f);
    secondPart.maximum = glm::vec3(2.5f, 0.5f, 2.5f);
    secondPart.valid = true;
    const glm::mat4 multipartTransform = glm::rotate(
        glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto firstWorldPart = core::transformBounds(firstPart, multipartTransform);
    const auto secondWorldPart = core::transformBounds(secondPart, multipartTransform);
    require(firstWorldPart.has_value() && secondWorldPart.has_value(),
            "valid multipart bounds could not be transformed");
    const std::array<geometry::AxisAlignedBounds, 2> worldParts = {
        *firstWorldPart, *secondWorldPart};
    const auto preciseMultipartBounds = core::mergeBounds(worldParts);
    const std::array<geometry::AxisAlignedBounds, 2> localParts = {
        firstPart, secondPart};
    const auto mergedLocalParts = core::mergeBounds(localParts);
    require(mergedLocalParts.has_value(),
            "valid multipart local bounds could not be merged");
    const auto inflatedMultipartBounds =
        core::transformBounds(*mergedLocalParts, multipartTransform);
    require(preciseMultipartBounds.has_value() && inflatedMultipartBounds.has_value() &&
                preciseMultipartBounds->maximum.z <
                    inflatedMultipartBounds->maximum.z * 0.5f,
            "multipart bounds were not tightened after per-part transforms");

    const std::vector<glm::vec3> trianglePositions = {
        {-1.0f, -1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 0.0f,  1.0f, 0.0f},
        {-1.0f, -1.0f, 2.0f},
        { 1.0f, -1.0f, 2.0f},
        { 0.0f,  1.0f, 2.0f}
    };
    const std::vector<std::uint32_t> singleTriangle = {0, 1, 2};
    const std::vector<std::uint32_t> twoTriangles = {0, 1, 2, 3, 4, 5};
    const auto triangleHit = core::intersectRayIndexedTriangles(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
        trianglePositions, twoTriangles, 0.1f, 100.0f);
    require(triangleHit.has_value() && almostEqual(*triangleHit, 3.0),
            "indexed triangle query did not return the nearest surface");
    require(!core::intersectRayIndexedTriangles(
                glm::vec3(0.9f, 0.9f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                trianglePositions, singleTriangle,
                0.1f, 100.0f)
                 .has_value(),
            "triangle query selected empty space inside its bounds");
    require(!core::intersectRayIndexedTriangles(
                glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                trianglePositions, singleTriangle,
                0.1f, 4.0f)
                 .has_value(),
            "triangle outside the camera clip range was accepted");

    geometry::AxisAlignedBounds triangleBounds;
    triangleBounds.minimum = glm::vec3(-1.0f, -1.0f, 0.0f);
    triangleBounds.maximum = glm::vec3(1.0f, 1.0f, 0.0f);
    triangleBounds.valid = true;
    const glm::mat4 triangleTransform = glm::translate(
        glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    const auto transformedTriangleHit = core::intersectRayTransformedIndexedMesh(
        glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
        triangleBounds, trianglePositions,
        singleTriangle, triangleTransform, 0.1f, 100.0f);
    require(transformedTriangleHit.has_value() &&
                almostEqual(*transformedTriangleHit, 4.0),
            "transformed triangle query returned the wrong world ray distance");

    geometry::AxisAlignedBounds reversedBounds = flatBounds;
    reversedBounds.minimum.x = 2.0f;
    reversedBounds.maximum.x = -2.0f;
    require(!core::intersectRayAabb(
                glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                reversedBounds)
                 .has_value(),
            "reversed bounds were accepted");

    const float infinity = (std::numeric_limits<float>::infinity)();
    require(!core::intersectRayAabb(
                glm::vec3(infinity, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                flatBounds)
                 .has_value(),
            "infinite ray origin was accepted");

    bounds.valid = false;
    require(!core::intersectRayAabb(
                glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), bounds)
                 .has_value(),
            "invalid bounds were accepted");
}

void testEditorCubePlacement() {
    const auto pointBounds = [](float x, float z) {
        geometry::AxisAlignedBounds bounds;
        bounds.minimum = glm::vec3(x, 0.0f, z);
        bounds.maximum = bounds.minimum;
        bounds.valid = true;
        return bounds;
    };

    const auto ordinaryDuplicate = core::calculateDuplicateObjectPosition(
        glm::vec3(2.0f, 3.0f, -4.0f), core::kMaxSceneCoordinate);
    require(ordinaryDuplicate.has_value() &&
                ordinaryDuplicate->x == 2.0f + core::kDuplicateObjectOffset &&
                ordinaryDuplicate->y == 3.0f &&
                ordinaryDuplicate->z == -4.0f + core::kDuplicateObjectOffset,
            "ordinary duplicate position used the wrong offset");
    const auto boundaryDuplicate = core::calculateDuplicateObjectPosition(
        glm::vec3(
            core::kMaxSceneCoordinate,
            -core::kMaxSceneCoordinate,
            core::kMaxSceneCoordinate),
        core::kMaxSceneCoordinate);
    require(boundaryDuplicate.has_value() &&
                boundaryDuplicate->x ==
                    core::kMaxSceneCoordinate - core::kDuplicateObjectOffset &&
                boundaryDuplicate->y == -core::kMaxSceneCoordinate &&
                boundaryDuplicate->z ==
                    core::kMaxSceneCoordinate - core::kDuplicateObjectOffset,
            "boundary duplicate position exceeded scene limits");
    require(!core::calculateDuplicateObjectPosition(
                 glm::vec3(core::kMaxSceneCoordinate + 1.0f, 0.0f, 0.0f),
                 core::kMaxSceneCoordinate)
                 .has_value() &&
                !core::calculateDuplicateObjectPosition(
                    glm::vec3(0.0f), 0.5f, 1.5f)
                    .has_value(),
            "duplicate placement accepted invalid source or offset bounds");

    const auto emptyPosition = core::findNearestFreeCubeGridPosition(
        glm::vec3(0.2f, 5.0f, -0.4f), {}, 100.0f, 4, 0.0f);
    require(emptyPosition.has_value() && *emptyPosition == glm::vec3(0.0f),
            "empty grid did not use the rounded base position");

    const std::array<geometry::AxisAlignedBounds, 4> occupiedNearest = {
        pointBounds(0.0f, 0.0f), pointBounds(-1.0f, 0.0f),
        pointBounds(0.0f, -1.0f), pointBounds(0.0f, 1.0f)};
    const auto nearestPosition = core::findNearestFreeCubeGridPosition(
        glm::vec3(0.0f), occupiedNearest, 100.0f, 4, 0.0f);
    require(nearestPosition.has_value() &&
                *nearestPosition == glm::vec3(1.0f, 0.0f, 0.0f),
            "placement skipped the nearest free grid cell");

    const std::array<geometry::AxisAlignedBounds, 1> occupiedBoundary = {
        pointBounds(5.0f, 0.0f)};
    const auto boundaryPosition = core::findNearestFreeCubeGridPosition(
        glm::vec3(5.0f, 0.0f, 0.0f), occupiedBoundary, 5.0f, 2, 0.0f);
    const glm::vec3 boundaryOffset = boundaryPosition.has_value()
        ? *boundaryPosition - glm::vec3(5.0f, 0.0f, 0.0f)
        : glm::vec3(0.0f);
    require(boundaryPosition.has_value() &&
                std::abs(boundaryPosition->x) <= 5.0f &&
                std::abs(boundaryPosition->z) <= 5.0f &&
                glm::dot(boundaryOffset, boundaryOffset) == 1.0f,
            "placement stopped at an out-of-range boundary candidate");

    geometry::AxisAlignedBounds blockedArea;
    blockedArea.minimum = glm::vec3(-10.0f, -1.0f, -10.0f);
    blockedArea.maximum = glm::vec3(10.0f, 1.0f, 10.0f);
    blockedArea.valid = true;
    const std::array<geometry::AxisAlignedBounds, 1> blockedBounds = {blockedArea};
    require(!core::findNearestFreeCubeGridPosition(
                 glm::vec3(0.0f), blockedBounds, 100.0f, 4, 0.1f)
                 .has_value(),
            "placement returned an occupied cell from a blocked search area");
    require(!core::findNearestFreeCubeGridPosition(
                 glm::vec3(1000.0f, 0.0f, 1000.0f), {}, 5.0f, 2)
                 .has_value(),
            "placement searched beyond its reachable coordinate range");
}

void testEditorObjectTransforms() {
    core::ObjectTransform original;
    original.position = glm::vec3(1.0f, 2.0f, 3.0f);
    original.rotationDeg = glm::vec3(10.0f, 175.0f, -20.0f);
    original.scale = glm::vec3(1.0f, 2.0f, 3.0f);

    const std::array repeatableCommands = {
        core::ObjectTransformCommand::MoveNegativeX,
        core::ObjectTransformCommand::MovePositiveY,
        core::ObjectTransformCommand::MoveNegativeZ,
        core::ObjectTransformCommand::RotatePositiveY,
        core::ObjectTransformCommand::ScaleDown};
    for (const core::ObjectTransformCommand command : repeatableCommands) {
        require(core::isRepeatableObjectTransform(command),
                "continuous transform command was not repeatable");
    }
    require(!core::isRepeatableObjectTransform(
                core::ObjectTransformCommand::Snap) &&
                !core::isRepeatableObjectTransform(
                    core::ObjectTransformCommand::Reset),
            "one-shot transform command was marked repeatable");

    const auto movedX = core::calculateObjectTransform(
        original, core::ObjectTransformCommand::MovePositiveX);
    require(movedX.has_value() && almostEqual(movedX->position.x, 1.5) &&
                movedX->position.y == original.position.y &&
                movedX->position.z == original.position.z,
            "positive X nudge changed the wrong transform components");

    const auto movedY = core::calculateObjectTransform(
        original, core::ObjectTransformCommand::MoveNegativeY);
    const auto movedZ = core::calculateObjectTransform(
        original, core::ObjectTransformCommand::MoveNegativeZ);
    require(movedY.has_value() && almostEqual(movedY->position.y, 1.5) &&
                movedZ.has_value() && almostEqual(movedZ->position.z, 2.5),
            "axis nudges used the wrong translation step");

    core::ObjectTransform boundary = original;
    boundary.position.x = core::kMaxSceneCoordinate;
    require(!core::calculateObjectTransform(
                 boundary, core::ObjectTransformCommand::MovePositiveX)
                 .has_value() &&
                core::calculateObjectTransform(
                    boundary, core::ObjectTransformCommand::MoveNegativeX)
                    .has_value(),
            "translation did not enforce the scene coordinate limit");

    const auto rotated = core::calculateObjectTransform(
        original, core::ObjectTransformCommand::RotatePositiveY);
    require(rotated.has_value() && almostEqual(rotated->rotationDeg.y, -170.0),
            "rotation step did not wrap into a stable degree range");
    const glm::vec3 wrapped = core::wrapEulerDegrees(
        glm::vec3(370.0f, -370.0f, 720.0f));
    require(almostEqual(wrapped.x, 10.0) && almostEqual(wrapped.y, -10.0) &&
                almostEqual(wrapped.z, 0.0),
            "Euler degree wrapping returned incorrect angles");

    const auto scaledUp = core::calculateObjectTransform(
        original, core::ObjectTransformCommand::ScaleUp);
    const auto scaledDown = core::calculateObjectTransform(
        original, core::ObjectTransformCommand::ScaleDown);
    require(scaledUp.has_value() && almostEqual(scaledUp->scale.x, 1.1) &&
                almostEqual(scaledUp->scale.y, 2.2) &&
                almostEqual(scaledUp->scale.z, 3.3) &&
                scaledDown.has_value() &&
                almostEqual(scaledDown->scale.x, 1.0 / 1.1),
            "uniform scale commands did not preserve component ratios");

    core::ObjectTransform maximumScale = original;
    maximumScale.scale = glm::vec3(core::kMaxSceneObjectScale);
    require(!core::calculateObjectTransform(
                 maximumScale, core::ObjectTransformCommand::ScaleUp)
                 .has_value(),
            "scale command exceeded the scene scale limit");

    core::ObjectTransform anisotropicMaximum = original;
    anisotropicMaximum.scale = glm::vec3(
        core::kMaxSceneObjectScale / 1.05f, 1.0f, 2.0f);
    const auto boundedScaleUp = core::calculateObjectTransform(
        anisotropicMaximum, core::ObjectTransformCommand::ScaleUp);
    require(boundedScaleUp.has_value() &&
                almostEqual(boundedScaleUp->scale.x, core::kMaxSceneObjectScale) &&
                almostEqual(
                    boundedScaleUp->scale.x / anisotropicMaximum.scale.x,
                    boundedScaleUp->scale.y / anisotropicMaximum.scale.y) &&
                almostEqual(
                    boundedScaleUp->scale.y / anisotropicMaximum.scale.y,
                    boundedScaleUp->scale.z / anisotropicMaximum.scale.z),
            "upper scale bound distorted a non-uniform object");

    core::ObjectTransform roundedMaximum = original;
    roundedMaximum.scale = glm::vec3(9090.998046875f, 1.0f, 2.0f);
    const auto roundedScaleUp = core::calculateObjectTransform(
        roundedMaximum, core::ObjectTransformCommand::ScaleUp);
    require(roundedScaleUp.has_value() &&
                roundedScaleUp->scale.x <= core::kMaxSceneObjectScale &&
                almostEqual(
                    roundedScaleUp->scale.x / roundedMaximum.scale.x,
                    roundedScaleUp->scale.y / roundedMaximum.scale.y),
            "float rounding rejected a valid bounded scale command");

    core::ObjectTransform anisotropicMinimum = original;
    anisotropicMinimum.scale = glm::vec3(
        core::kMinSceneObjectScale * 1.05f, 1.0f, 2.0f);
    const auto boundedScaleDown = core::calculateObjectTransform(
        anisotropicMinimum, core::ObjectTransformCommand::ScaleDown);
    require(boundedScaleDown.has_value() &&
                almostEqual(
                    boundedScaleDown->scale.x, core::kMinSceneObjectScale) &&
                almostEqual(
                    boundedScaleDown->scale.x / anisotropicMinimum.scale.x,
                    boundedScaleDown->scale.y / anisotropicMinimum.scale.y) &&
                almostEqual(
                    boundedScaleDown->scale.y / anisotropicMinimum.scale.y,
                    boundedScaleDown->scale.z / anisotropicMinimum.scale.z),
            "lower scale bound distorted a non-uniform object");

    core::ObjectTransform unsnapped = original;
    unsnapped.position = glm::vec3(0.24f, 0.26f, -0.74f);
    unsnapped.rotationDeg = glm::vec3(7.4f, 22.6f, -179.0f);
    const auto snapped = core::calculateObjectTransform(
        unsnapped, core::ObjectTransformCommand::Snap);
    require(snapped.has_value() && almostEqual(snapped->position.x, 0.0) &&
                almostEqual(snapped->position.y, 0.5) &&
                almostEqual(snapped->position.z, -0.5) &&
                almostEqual(snapped->rotationDeg.x, 0.0) &&
                almostEqual(snapped->rotationDeg.y, 30.0) &&
                almostEqual(std::abs(snapped->rotationDeg.z), 180.0),
            "transform snapping did not use the editor grid steps");

    core::ObjectTransform boundarySnap = original;
    boundarySnap.position = glm::vec3(
        core::kMaxSceneCoordinate, -core::kMaxSceneCoordinate, 0.0f);
    const auto snappedBoundary = core::calculateObjectTransform(
        boundarySnap, core::ObjectTransformCommand::Snap,
        1.5f, 15.0f, 1.1f);
    require(snappedBoundary.has_value() &&
                almostEqual(snappedBoundary->position.x, 999'999.0) &&
                almostEqual(snappedBoundary->position.y, -999'999.0),
            "boundary snap rejected the nearest valid custom grid point");

    const auto reset = core::calculateObjectTransform(
        original, core::ObjectTransformCommand::Reset);
    require(reset.has_value() && reset->position == glm::vec3(0.0f) &&
                reset->rotationDeg == glm::vec3(0.0f) &&
                reset->scale == glm::vec3(1.0f),
            "transform reset did not restore identity values");
    core::ObjectTransform identity;
    require(!core::calculateObjectTransform(
                 identity, core::ObjectTransformCommand::Reset)
                 .has_value(),
            "identity transform reset created a false history change");

    core::ObjectTransform invalid = original;
    invalid.position.x = (std::numeric_limits<float>::quiet_NaN)();
    require(!core::calculateObjectTransform(
                 invalid, core::ObjectTransformCommand::MovePositiveX)
                 .has_value() &&
                !core::calculateObjectTransform(
                    original, core::ObjectTransformCommand::ScaleUp,
                    0.5f, 15.0f, 1.0f)
                    .has_value(),
            "invalid transform command parameters were accepted");
}

void testEditorCameraFraming() {
    require(almostEqual(core::clampEditorCameraDelta(0.016f), 0.016) &&
                almostEqual(
                    core::clampEditorCameraDelta(5.0f),
                    core::kMaximumEditorCameraDeltaSeconds) &&
                core::clampEditorCameraDelta(-1.0f) == 0.0f &&
                core::clampEditorCameraDelta(
                    std::numeric_limits<float>::quiet_NaN()) == 0.0f,
            "camera delta clamp accepted a stall or invalid interval");
    require(core::shouldProcessEditorCameraScroll(true, false) &&
                core::shouldProcessEditorCameraScroll(false, true) &&
                !core::shouldProcessEditorCameraScroll(false, false),
            "camera scroll gating ignored active look or viewport bounds");

    geometry::AxisAlignedBounds bounds;
    bounds.minimum = glm::vec3(-1.0f);
    bounds.maximum = glm::vec3(1.0f);
    bounds.valid = true;

    const auto widePosition = core::calculateFramedCameraPosition(
        bounds, glm::vec3(0.0f, 0.0f, -1.0f), 45.0f, 16.0f / 9.0f,
        0.1f, 100.0f);
    require(widePosition.has_value() && almostEqual(widePosition->x, 0.0) &&
                almostEqual(widePosition->y, 0.0) && widePosition->z > 3.0f &&
                widePosition->z < 10.0f,
            "wide camera framing returned an invalid position");
    requireBoundsInsideFrustum(
        bounds, *widePosition, glm::vec3(0.0f, 0.0f, -1.0f),
        45.0f, 16.0f / 9.0f, 0.1f, 100.0f, 1.15f,
        "wide camera framing");

    const auto narrowPosition = core::calculateFramedCameraPosition(
        bounds, glm::vec3(0.0f, 0.0f, -1.0f), 45.0f, 0.5f,
        0.1f, 100.0f);
    require(narrowPosition.has_value() && narrowPosition->z > widePosition->z,
            "narrow viewport did not increase framing distance");
    requireBoundsInsideFrustum(
        bounds, *narrowPosition, glm::vec3(0.0f, 0.0f, -1.0f),
        45.0f, 0.5f, 0.1f, 100.0f, 1.15f,
        "narrow camera framing");

    geometry::AxisAlignedBounds offsetBounds = bounds;
    offsetBounds.minimum += glm::vec3(8.0f, 2.0f, -3.0f);
    offsetBounds.maximum += glm::vec3(8.0f, 2.0f, -3.0f);
    const auto offsetPosition = core::calculateFramedCameraPosition(
        offsetBounds, glm::vec3(1.0f, 0.0f, 0.0f), 60.0f, 1.0f,
        0.1f, 100.0f);
    require(offsetPosition.has_value() && offsetPosition->x < 8.0f &&
                almostEqual(offsetPosition->y, 2.0) &&
                almostEqual(offsetPosition->z, -3.0),
            "offset object was not framed along the view direction");
    requireBoundsInsideFrustum(
        offsetBounds, *offsetPosition, glm::vec3(1.0f, 0.0f, 0.0f),
        60.0f, 1.0f, 0.1f, 100.0f, 1.15f,
        "offset camera framing");

    geometry::AxisAlignedBounds elongatedBounds;
    elongatedBounds.minimum = glm::vec3(-0.5f, -0.5f, -40.0f);
    elongatedBounds.maximum = glm::vec3(0.5f, 0.5f, 40.0f);
    elongatedBounds.valid = true;
    const auto elongatedPosition = core::calculateFramedCameraPosition(
        elongatedBounds, glm::vec3(0.0f, 0.0f, -1.0f),
        45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    require(elongatedPosition.has_value(),
            "valid elongated object was rejected by camera framing");
    requireBoundsInsideFrustum(
        elongatedBounds, *elongatedPosition, glm::vec3(0.0f, 0.0f, -1.0f),
        45.0f, 16.0f / 9.0f, 0.1f, 100.0f, 1.15f,
        "elongated camera framing");

    geometry::AxisAlignedBounds arbitraryBounds;
    arbitraryBounds.minimum = glm::vec3(6.0f, -3.0f, 2.0f);
    arbitraryBounds.maximum = glm::vec3(11.0f, 4.0f, 8.0f);
    arbitraryBounds.valid = true;
    const glm::vec3 arbitraryDirection =
        glm::normalize(glm::vec3(-1.0f, -0.4f, -0.5f));
    const auto arbitraryPosition = core::calculateFramedCameraPosition(
        arbitraryBounds, arbitraryDirection, 52.0f, 1.7f, 0.1f, 100.0f);
    require(arbitraryPosition.has_value(),
            "arbitrarily oriented camera could not frame valid bounds");
    requireBoundsInsideFrustum(
        arbitraryBounds, *arbitraryPosition, arbitraryDirection,
        52.0f, 1.7f, 0.1f, 100.0f, 1.15f,
        "arbitrary camera framing");

    const glm::vec3 nearVerticalDirection = glm::normalize(
        glm::vec3(0.01f, 0.9998f, -0.015f));
    const auto nearVerticalPosition = core::calculateFramedCameraPosition(
        bounds, nearVerticalDirection, 45.0f, 16.0f / 9.0f,
        0.1f, 100.0f);
    require(nearVerticalPosition.has_value(),
            "near-vertical camera direction was rejected");
    requireBoundsInsideFrustum(
        bounds, *nearVerticalPosition, nearVerticalDirection,
        45.0f, 16.0f / 9.0f, 0.1f, 100.0f, 1.15f,
        "near-vertical camera framing");

    require(!core::calculateFramedCameraPosition(
                bounds, glm::vec3(0.0f), 45.0f, 1.0f, 0.1f, 100.0f)
                 .has_value(),
            "zero camera direction was accepted");

    geometry::AxisAlignedBounds hugeBounds;
    hugeBounds.minimum = glm::vec3(-50.0f);
    hugeBounds.maximum = glm::vec3(50.0f);
    hugeBounds.valid = true;
    require(!core::calculateFramedCameraPosition(
                hugeBounds, glm::vec3(0.0f, 0.0f, -1.0f),
                45.0f, 1.0f, 0.1f, 100.0f)
                 .has_value(),
            "object larger than the camera range was accepted");
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"asset path containment", testAssetPathContainment},
        {"editor layout and hit testing", testEditorLayoutAndHitTesting},
        {"editor translation gizmo", testEditorTranslationGizmo},
        {"window settings persistence", testWindowSettingsPersistence},
        {"benchmark statistics", testBenchmarkStatistics},
        {"benchmark JSON/CSV round-trip", testBenchmarkRoundTripAndMultilineCsv},
        {"benchmark invalid report rejection", testBenchmarkRejectsInvalidReports},
        {"benchmark invalid comparison rejection", testBenchmarkRejectsInvalidComparison},
        {"scene validation matrix", testSceneValidationMatrix},
        {"scene I/O round-trip", testSceneIoRoundTrip},
        {"oversized scene rejection", testOversizedSceneIsNotSaved},
        {"scene history transactions", testSceneHistoryTransactions},
        {"runtime object lookup", testRuntimeObjectLookup},
        {"spatial ray queries", testSpatialQueries},
        {"editor cube placement", testEditorCubePlacement},
        {"editor object transforms", testEditorObjectTransforms},
        {"editor camera framing", testEditorCameraFraming}
    };

    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& exception) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << exception.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All core tests passed\n";
    return 0;
}
