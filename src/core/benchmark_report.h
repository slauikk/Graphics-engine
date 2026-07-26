#ifndef BENCHMARK_REPORT_H
#define BENCHMARK_REPORT_H

#include <filesystem>
#include <string>
#include <vector>

namespace core {

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
    std::vector<float> samplesMs;
    double medianMs = 0.0;
    double p95Ms = 0.0;
    double meanMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;
};

struct BenchmarkReportResult {
    bool success = false;
    std::filesystem::path jsonPath;
    std::filesystem::path csvPath;
    std::string timestampUtc;
    std::string error;
};

BenchmarkReportResult writeBenchmarkReport(
    const BenchmarkReport& report,
    const std::filesystem::path& outputDirectory);

} // namespace core

#endif // BENCHMARK_REPORT_H
