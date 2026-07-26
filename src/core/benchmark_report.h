#ifndef BENCHMARK_REPORT_H
#define BENCHMARK_REPORT_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace core {

struct BenchmarkStatistics {
    double medianMs = 0.0;
    double p95Ms = 0.0;
    double meanMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;
};

struct BenchmarkMetric {
    std::vector<float> samplesMs;
    BenchmarkStatistics statistics;
};

struct BenchmarkReport {
    std::string workload;
    std::string gpuVendor;
    std::string gpuRenderer;
    std::string openGlVersion;
    int targetWidth = 0;
    int targetHeight = 0;
    int displayWidth = 0;
    int displayHeight = 0;
    int instances = 0;
    int shaderIterations = 0;
    double warmupSeconds = 0.0;
    BenchmarkMetric draw;
    BenchmarkMetric frameInterval;
    BenchmarkMetric cpuWork;
    BenchmarkMetric present;
    BenchmarkMetric gpuTotal;
    BenchmarkMetric gpuScene;
    BenchmarkMetric gpuUi;
};

struct BenchmarkReportResult {
    bool success = false;
    std::filesystem::path jsonPath;
    std::filesystem::path csvPath;
    std::string timestampUtc;
    std::string error;
};

struct BenchmarkComparison {
    std::string timestampUtc;
    std::string gpuVendor;
    std::string gpuRenderer;
    BenchmarkStatistics draw;
    BenchmarkStatistics frameInterval;
    BenchmarkStatistics cpuWork;
    BenchmarkStatistics present;
    BenchmarkStatistics gpuTotal;
};

BenchmarkStatistics calculateBenchmarkStatistics(const std::vector<float>& samplesMs);
BenchmarkMetric makeBenchmarkMetric(std::vector<float> samplesMs);

BenchmarkReportResult writeBenchmarkReport(
    const BenchmarkReport& report,
    const std::filesystem::path& outputDirectory);

std::optional<BenchmarkComparison> findLatestCompatibleGpuComparison(
    const BenchmarkReport& currentReport,
    const BenchmarkReportResult& currentResult);

} // namespace core

#endif // BENCHMARK_REPORT_H
