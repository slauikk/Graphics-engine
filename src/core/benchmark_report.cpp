#include "benchmark_report.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <locale>
#include <numeric>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>

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

bool metricIsValid(const core::BenchmarkMetric& metric) {
    if (metric.samplesMs.empty()) {
        return false;
    }

    const bool samplesAreFinite = std::all_of(
        metric.samplesMs.begin(), metric.samplesMs.end(),
        [](float sample) { return std::isfinite(sample) && sample >= 0.0f; });
    if (!samplesAreFinite) {
        return false;
    }

    const auto almostEqual = [](double left, double right) {
        const double scale = std::max({1.0, std::abs(left), std::abs(right)});
        return std::abs(left - right) <= 0.000001 * scale;
    };
    const auto& statistics = metric.statistics;
    const core::BenchmarkStatistics expected =
        core::calculateBenchmarkStatistics(metric.samplesMs);
    return std::isfinite(statistics.medianMs) &&
           std::isfinite(statistics.p95Ms) && std::isfinite(statistics.meanMs) &&
           std::isfinite(statistics.minMs) && std::isfinite(statistics.maxMs) &&
           almostEqual(statistics.medianMs, expected.medianMs) &&
           almostEqual(statistics.p95Ms, expected.p95Ms) &&
           almostEqual(statistics.meanMs, expected.meanMs) &&
           almostEqual(statistics.minMs, expected.minMs) &&
           almostEqual(statistics.maxMs, expected.maxMs);
}

bool reportIsValid(const core::BenchmarkReport& report) {
    if (report.targetWidth <= 0 || report.targetHeight <= 0 ||
        report.displayWidth <= 0 || report.displayHeight <= 0 ||
        report.instances <= 0 || report.shaderIterations <= 0 ||
        !std::isfinite(report.warmupSeconds) || report.warmupSeconds < 0.0) {
        return false;
    }

    const std::size_t sampleCount = report.draw.samplesMs.size();
    const core::BenchmarkMetric* metrics[] = {
        &report.draw, &report.frameInterval, &report.cpuWork, &report.present,
        &report.gpuTotal, &report.gpuScene, &report.gpuUi
    };
    return std::all_of(std::begin(metrics), std::end(metrics),
                       [sampleCount](const core::BenchmarkMetric* metric) {
                           return metricIsValid(*metric) &&
                                  metric->samplesMs.size() == sampleCount;
                       });
}

void writeJsonMetric(std::ostream& output, std::string_view name,
                     const core::BenchmarkMetric& metric, bool trailingComma) {
    const auto& statistics = metric.statistics;
    output << "    \"" << name << "\": {\n"
           << "      \"median\": " << statistics.medianMs << ",\n"
           << "      \"p95\": " << statistics.p95Ms << ",\n"
           << "      \"mean\": " << statistics.meanMs << ",\n"
           << "      \"min\": " << statistics.minMs << ",\n"
           << "      \"max\": " << statistics.maxMs << ",\n"
           << "      \"samples\": [";

    for (std::size_t index = 0; index < metric.samplesMs.size(); ++index) {
        if (index != 0) {
            output << ',';
        }
        if (index % 10 == 0) {
            output << "\n        ";
        } else {
            output << ' ';
        }
        output << metric.samplesMs[index];
    }
    output << "\n      ]\n    }" << (trailingComma ? ",\n" : "\n");
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
    output << "{\n  \"schema_version\": 2,\n  \"timestamp_utc\": ";
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
           << "  \"sample_count\": " << report.draw.samplesMs.size() << ",\n"
           << "  \"metrics_ms\": {\n";
    writeJsonMetric(output, "draw", report.draw, true);
    writeJsonMetric(output, "frame_interval", report.frameInterval, true);
    writeJsonMetric(output, "cpu_work", report.cpuWork, true);
    writeJsonMetric(output, "present", report.present, true);
    writeJsonMetric(output, "gpu_total", report.gpuTotal, true);
    writeJsonMetric(output, "gpu_scene", report.gpuScene, true);
    writeJsonMetric(output, "gpu_ui", report.gpuUi, false);
    output << "  }\n}\n";
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
                  "shader_iterations,warmup_seconds,sample_count,draw_median_ms,"
                  "draw_p95_ms,frame_interval_median_ms,cpu_work_median_ms,"
                  "present_median_ms,gpu_total_median_ms,gpu_scene_median_ms,"
                  "gpu_ui_median_ms,json_file\n";
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
           << ',' << report.draw.samplesMs.size()
           << ',' << report.draw.statistics.medianMs
           << ',' << report.draw.statistics.p95Ms
           << ',' << report.frameInterval.statistics.medianMs
           << ',' << report.cpuWork.statistics.medianMs
           << ',' << report.present.statistics.medianMs
           << ',' << report.gpuTotal.statistics.medianMs
           << ',' << report.gpuScene.statistics.medianMs
           << ',' << report.gpuUi.statistics.medianMs
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

std::optional<std::vector<std::string>> parseCsvLine(std::string_view line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char character = line[index];
        if (quoted) {
            if (character == '"') {
                if (index + 1 < line.size() && line[index + 1] == '"') {
                    field.push_back('"');
                    ++index;
                } else {
                    quoted = false;
                }
            } else {
                field.push_back(character);
            }
        } else if (character == '"') {
            if (!field.empty()) {
                return std::nullopt;
            }
            quoted = true;
        } else if (character == ',') {
            fields.push_back(std::move(field));
            field.clear();
        } else if (character != '\r') {
            field.push_back(character);
        }
    }

    if (quoted) {
        return std::nullopt;
    }
    fields.push_back(std::move(field));
    return fields;
}

bool readCsvRecord(std::istream& input, std::string& record) {
    record.clear();
    bool quoted = false;
    char character = '\0';
    while (input.get(character)) {
        if (character == '"') {
            record.push_back(character);
            if (quoted && input.peek() == '"') {
                record.push_back(static_cast<char>(input.get()));
            } else {
                quoted = !quoted;
            }
        } else if (character == '\n' && !quoted) {
            return true;
        } else if (character != '\r' || quoted) {
            record.push_back(character);
        }
    }
    return !record.empty();
}

template <typename Number>
bool parseNumber(std::string_view value, Number& result) {
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    const auto conversion = std::from_chars(begin, end, result);
    return conversion.ec == std::errc() && conversion.ptr == end;
}

bool summaryIsCompatible(const std::vector<std::string>& fields,
                         const core::BenchmarkReport& current) {
    if (fields.size() != 22 || fields[0] == "timestamp_utc" ||
        fields[4] != current.workload || fields[2] == current.gpuRenderer) {
        return false;
    }

    int targetWidth = 0;
    int targetHeight = 0;
    int instances = 0;
    int shaderIterations = 0;
    std::size_t sampleCount = 0;
    return parseNumber(fields[5], targetWidth) && targetWidth == current.targetWidth &&
           parseNumber(fields[6], targetHeight) && targetHeight == current.targetHeight &&
           parseNumber(fields[9], instances) && instances == current.instances &&
           parseNumber(fields[10], shaderIterations) &&
               shaderIterations == current.shaderIterations &&
           parseNumber(fields[12], sampleCount) &&
               sampleCount == current.draw.samplesMs.size();
}

std::optional<core::BenchmarkComparison> comparisonFromFields(
    const std::vector<std::string>& fields) {
    core::BenchmarkComparison comparison;
    comparison.timestampUtc = fields[0];
    comparison.gpuVendor = fields[1];
    comparison.gpuRenderer = fields[2];

    if (!parseNumber(fields[13], comparison.draw.medianMs) ||
        !parseNumber(fields[14], comparison.draw.p95Ms) ||
        !parseNumber(fields[15], comparison.frameInterval.medianMs) ||
        !parseNumber(fields[16], comparison.cpuWork.medianMs) ||
        !parseNumber(fields[17], comparison.present.medianMs) ||
        !parseNumber(fields[18], comparison.gpuTotal.medianMs)) {
        return std::nullopt;
    }
    const double parsedStatistics[] = {
        comparison.draw.medianMs,
        comparison.draw.p95Ms,
        comparison.frameInterval.medianMs,
        comparison.cpuWork.medianMs,
        comparison.present.medianMs,
        comparison.gpuTotal.medianMs
    };
    if (!std::all_of(std::begin(parsedStatistics), std::end(parsedStatistics),
                     [](double value) {
                         return std::isfinite(value) && value >= 0.0;
                     })) {
        return std::nullopt;
    }
    return comparison;
}

} // namespace

namespace core {

BenchmarkStatistics calculateBenchmarkStatistics(const std::vector<float>& samplesMs) {
    BenchmarkStatistics statistics;
    if (samplesMs.empty()) {
        return statistics;
    }

    std::vector<float> sortedSamples(samplesMs.begin(), samplesMs.end());
    std::sort(sortedSamples.begin(), sortedSamples.end());
    const std::size_t sampleCount = sortedSamples.size();
    const std::size_t middle = sampleCount / 2;
    statistics.medianMs = sampleCount % 2 == 0
        ? (static_cast<double>(sortedSamples[middle - 1]) + sortedSamples[middle]) * 0.5
        : sortedSamples[middle];
    const std::size_t p95Index = static_cast<std::size_t>(
        std::ceil(static_cast<double>(sampleCount) * 0.95)) - 1;
    statistics.p95Ms = sortedSamples[p95Index];
    statistics.meanMs = std::accumulate(samplesMs.begin(), samplesMs.end(), 0.0) /
                        static_cast<double>(sampleCount);
    statistics.minMs = sortedSamples.front();
    statistics.maxMs = sortedSamples.back();
    return statistics;
}

BenchmarkMetric makeBenchmarkMetric(std::vector<float> samplesMs) {
    BenchmarkMetric metric;
    metric.statistics = calculateBenchmarkStatistics(samplesMs);
    metric.samplesMs = std::move(samplesMs);
    return metric;
}

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
    result.csvPath = outputDirectory / "benchmark_summary_v2.csv";

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

std::optional<BenchmarkComparison> findLatestCompatibleGpuComparison(
    const BenchmarkReport& currentReport,
    const BenchmarkReportResult& currentResult) {
    if (!currentResult.success || currentResult.csvPath.empty()) {
        return std::nullopt;
    }

    std::ifstream input(currentResult.csvPath, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }

    std::optional<BenchmarkComparison> latest;
    std::string line;
    while (readCsvRecord(input, line)) {
        const auto fields = parseCsvLine(line);
        if (!fields || !summaryIsCompatible(*fields, currentReport) ||
            (*fields)[0] == currentResult.timestampUtc) {
            continue;
        }
        if (auto comparison = comparisonFromFields(*fields)) {
            latest = std::move(comparison);
        }
    }
    return latest;
}

} // namespace core
