#include "core/benchmark_report.h"
#include "core/scene_document.h"

#include <algorithm>
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

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"benchmark statistics", testBenchmarkStatistics},
        {"benchmark JSON/CSV round-trip", testBenchmarkRoundTripAndMultilineCsv},
        {"benchmark invalid report rejection", testBenchmarkRejectsInvalidReports},
        {"benchmark invalid comparison rejection", testBenchmarkRejectsInvalidComparison},
        {"scene validation matrix", testSceneValidationMatrix},
        {"scene I/O round-trip", testSceneIoRoundTrip},
        {"oversized scene rejection", testOversizedSceneIsNotSaved}
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
