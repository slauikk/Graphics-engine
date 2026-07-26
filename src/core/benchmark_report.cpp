#include "benchmark_report.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string_view>

namespace {

struct ReportTimestamp {
    std::string utc;
    std::string filePart;
};

ReportTimestamp makeTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utcTime{};
#ifdef _WIN32
    gmtime_s(&utcTime, &time);
#else
    gmtime_r(&time, &utcTime);
#endif

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    const int milliseconds = static_cast<int>(elapsed.count() % 1000);

    std::ostringstream utc;
    utc.imbue(std::locale::classic());
    utc << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << milliseconds << 'Z';

    std::ostringstream filePart;
    filePart.imbue(std::locale::classic());
    filePart << std::put_time(&utcTime, "%Y%m%d_%H%M%S")
             << '_' << std::setw(3) << std::setfill('0') << milliseconds;

    return {utc.str(), filePart.str()};
}

void writeJsonString(std::ostream& output, std::string_view value) {
    static constexpr char kHexDigits[] = "0123456789abcdef";

    output << '"';
    for (const unsigned char character : value) {
        switch (character) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character < 0x20) {
                    output << "\\u00"
                           << kHexDigits[(character >> 4) & 0x0f]
                           << kHexDigits[character & 0x0f];
                } else {
                    output << static_cast<char>(character);
                }
                break;
        }
    }
    output << '"';
}

void writeCsvString(std::ostream& output, std::string_view value) {
    const bool needsQuotes = value.find_first_of(",\"\r\n") != std::string_view::npos;
    if (!needsQuotes) {
        output << value;
        return;
    }

    output << '"';
    for (const char character : value) {
        if (character == '"') {
            output << "\"\"";
        } else {
            output << character;
        }
    }
    output << '"';
}

bool reportIsValid(const core::BenchmarkReport& report) {
    if (report.samplesMs.empty() || report.targetWidth <= 0 || report.targetHeight <= 0 ||
        report.instances <= 0 || report.shaderIterations <= 0) {
        return false;
    }

    const bool samplesAreFinite = std::all_of(
        report.samplesMs.begin(), report.samplesMs.end(),
        [](float sample) { return std::isfinite(sample) && sample >= 0.0f; });
    return samplesAreFinite && std::isfinite(report.medianMs) &&
           std::isfinite(report.p95Ms) && std::isfinite(report.meanMs) &&
           std::isfinite(report.minMs) && std::isfinite(report.maxMs);
}

bool writeJsonFile(
    const core::BenchmarkReport& report,
    const ReportTimestamp& timestamp,
    const std::filesystem::path& path,
    std::string& errorMessage) {
    std::filesystem::path temporaryPath = path;
    temporaryPath += ".tmp";

    std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!output) {
        errorMessage = "could not create JSON file";
        return false;
    }

    output.imbue(std::locale::classic());
    output << std::fixed << std::setprecision(6);
    output << "{\n  \"schema_version\": 1,\n  \"timestamp_utc\": ";
    writeJsonString(output, timestamp.utc);
    output << ",\n  \"workload\": ";
    writeJsonString(output, report.workload);
    output << ",\n  \"gpu\": {\n    \"vendor\": ";
    writeJsonString(output, report.gpuVendor);
    output << ",\n    \"renderer\": ";
    writeJsonString(output, report.gpuRenderer);
    output << ",\n    \"opengl_version\": ";
    writeJsonString(output, report.openGlVersion);
    output << "\n  },\n  \"target\": {\"width\": " << report.targetWidth
           << ", \"height\": " << report.targetHeight << "},\n"
           << "  \"display\": {\"width\": " << report.displayWidth
           << ", \"height\": " << report.displayHeight << "},\n"
           << "  \"instances\": " << report.instances << ",\n"
           << "  \"shader_iterations\": " << report.shaderIterations << ",\n"
           << "  \"warmup_seconds\": " << report.warmupSeconds << ",\n"
           << "  \"sample_count\": " << report.samplesMs.size() << ",\n"
           << "  \"statistics_ms\": {\n"
           << "    \"median\": " << report.medianMs << ",\n"
           << "    \"p95\": " << report.p95Ms << ",\n"
           << "    \"mean\": " << report.meanMs << ",\n"
           << "    \"min\": " << report.minMs << ",\n"
           << "    \"max\": " << report.maxMs << "\n"
           << "  },\n  \"samples_ms\": [";

    for (std::size_t index = 0; index < report.samplesMs.size(); ++index) {
        if (index != 0) {
            output << ',';
        }
        if (index % 10 == 0) {
            output << "\n    ";
        } else {
            output << ' ';
        }
        output << report.samplesMs[index];
    }
    output << "\n  ]\n}\n";
    output.flush();

    if (!output) {
        errorMessage = "could not finish writing JSON file";
        output.close();
        std::error_code removeError;
        std::filesystem::remove(temporaryPath, removeError);
        return false;
    }
    output.close();

    std::error_code renameError;
    std::filesystem::rename(temporaryPath, path, renameError);
    if (renameError) {
        errorMessage = "could not finalize JSON file: " + renameError.message();
        std::error_code removeError;
        std::filesystem::remove(temporaryPath, removeError);
        return false;
    }

    return true;
}

bool appendCsvSummary(
    const core::BenchmarkReport& report,
    const ReportTimestamp& timestamp,
    const std::filesystem::path& jsonPath,
    const std::filesystem::path& csvPath,
    std::string& errorMessage) {
    std::error_code filesystemError;
    bool writeHeader = true;
    if (std::filesystem::exists(csvPath, filesystemError)) {
        if (filesystemError) {
            errorMessage = "could not inspect CSV file: " + filesystemError.message();
            return false;
        }
        writeHeader = std::filesystem::file_size(csvPath, filesystemError) == 0;
        if (filesystemError) {
            errorMessage = "could not inspect CSV size: " + filesystemError.message();
            return false;
        }
    } else if (filesystemError) {
        errorMessage = "could not inspect CSV file: " + filesystemError.message();
        return false;
    }

    std::ofstream output(csvPath, std::ios::binary | std::ios::app);
    if (!output) {
        errorMessage = "could not open CSV summary";
        return false;
    }

    output.imbue(std::locale::classic());
    if (writeHeader) {
        output << "timestamp_utc,gpu_vendor,gpu_renderer,opengl_version,workload,"
                  "target_width,target_height,display_width,display_height,instances,"
                  "shader_iterations,warmup_seconds,sample_count,median_ms,p95_ms,"
                  "mean_ms,min_ms,max_ms,json_file\n";
    }

    writeCsvString(output, timestamp.utc);
    output << ',';
    writeCsvString(output, report.gpuVendor);
    output << ',';
    writeCsvString(output, report.gpuRenderer);
    output << ',';
    writeCsvString(output, report.openGlVersion);
    output << ',';
    writeCsvString(output, report.workload);
    output << ',' << report.targetWidth
           << ',' << report.targetHeight
           << ',' << report.displayWidth
           << ',' << report.displayHeight
           << ',' << report.instances
           << ',' << report.shaderIterations
           << ',' << std::fixed << std::setprecision(6) << report.warmupSeconds
           << ',' << report.samplesMs.size()
           << ',' << report.medianMs
           << ',' << report.p95Ms
           << ',' << report.meanMs
           << ',' << report.minMs
           << ',' << report.maxMs
           << ',';
    writeCsvString(output, jsonPath.filename().string());
    output << '\n';
    output.flush();

    if (!output) {
        errorMessage = "could not append CSV summary";
        return false;
    }
    return true;
}

} // namespace

namespace core {

BenchmarkReportResult writeBenchmarkReport(
    const BenchmarkReport& report,
    const std::filesystem::path& outputDirectory) {
    BenchmarkReportResult result;
    if (!reportIsValid(report)) {
        result.error = "benchmark report contains invalid values";
        return result;
    }

    std::error_code filesystemError;
    std::filesystem::create_directories(outputDirectory, filesystemError);
    if (filesystemError) {
        result.error = "could not create report directory: " + filesystemError.message();
        return result;
    }

    const ReportTimestamp timestamp = makeTimestamp();
    result.timestampUtc = timestamp.utc;
    result.jsonPath = outputDirectory / ("benchmark_" + timestamp.filePart + ".json");
    result.csvPath = outputDirectory / "benchmark_summary.csv";

    if (!writeJsonFile(report, timestamp, result.jsonPath, result.error)) {
        return result;
    }
    if (!appendCsvSummary(report, timestamp, result.jsonPath, result.csvPath, result.error)) {
        result.error = "JSON saved, but " + result.error;
        return result;
    }

    result.success = true;
    return result;
}

} // namespace core
