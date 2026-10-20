#include "loginsight/app/analysis_service.hpp"
#include "loginsight/parsers/plain_text_log_parser.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace loginsight::app {
AnalysisService::AnalysisService() {
  if (!registry_.registerParser(std::make_unique<parsers::PlainTextLogParser>())) {
    throw std::runtime_error("unable to configure plain-text parser");
  }
}

AnalysisResult AnalysisService::analyze(const std::filesystem::path& path) const {
  return analyze_file(path, parsers::FileImportService{registry_});
}

AnalysisResult analyze_file(const std::filesystem::path& path,
                            const parsers::FileImportService& importer) {
  AnalysisResult result{.path = path,
                        .status = AnalysisStatus::io_failure,
                        .parsed = {},
                        .summary = {},
                        .diagnostic = {}};
  std::error_code error;
  const auto status = std::filesystem::status(path, error);
  if (error == std::errc::no_such_file_or_directory || (!error && !exists(status))) {
    result.status = AnalysisStatus::file_not_found;
    result.diagnostic = "file not found";
  } else if (error || !is_regular_file(status)) {
    result.diagnostic = "unable to open file";
  } else {
    auto imported = importer.importFile(path);
    if (!imported.isSupported()) {
      result.status = AnalysisStatus::unsupported_format;
      result.diagnostic = "unsupported file format";
    } else {
      result.parsed = std::move(imported.parseResult);
      if (std::any_of(result.parsed.errors.begin(), result.parsed.errors.end(),
                      [](const auto& e) { return e.lineNumber == 0U; })) {
        result.diagnostic = "unable to open or completely read file";
      } else {
        result.status = AnalysisStatus::completed;
        result.summary = parsers::build_analysis_summary(result.parsed);
        for (auto& entry : result.parsed.entries) {
          entry.source.file_path = path.string();
        }
      }
    }
  }
  return result;
}

std::string format_timestamp(core::LogTimestamp timestamp) {
  const auto day = std::chrono::floor<std::chrono::days>(timestamp);
  const std::chrono::year_month_day date{day};
  const std::chrono::hh_mm_ss time{std::chrono::floor<std::chrono::seconds>(timestamp) - day};
  std::ostringstream out;
  out << std::setfill('0') << std::setw(4) << int(date.year()) << '-' << std::setw(2)
      << unsigned(date.month()) << '-' << std::setw(2) << unsigned(date.day()) << 'T'
      << std::setw(2) << time.hours().count() << ':' << std::setw(2) << time.minutes().count()
      << ':' << std::setw(2) << time.seconds().count() << 'Z';
  return out.str();
}

namespace {
using Json = nlohmann::json;
void emit(std::ostream& out, const Json& value) {
  // Display/export invalid UTF-8 as replacement characters rather than aborting an import.
  out << value.dump(-1, ' ', false, Json::error_handler_t::replace);
}
struct TemporaryDirectory {
  std::filesystem::path path;
  ~TemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};
} // namespace

void write_json_report(std::ostream& out, std::span<const AnalysisResult> results) {
  out << "{\"schema_version\":1,\"files\":[";
  bool first_file = true;
  for (const auto& file : results) {
    if (!first_file)
      out << ',';
    first_file = false;
    out << "{\"path\":";
    emit(out, file.path.string());
    out << ",\"completed\":" << (file.completed() ? "true" : "false") << ",\"diagnostic\":";
    emit(out, file.diagnostic);
    const auto& s = file.summary;
    out << ",\"summary\":";
    emit(out, Json{{"valid_entries", s.validEntryCount},
                   {"malformed_records", s.malformedLineCount},
                   {"trace", s.traceCount},
                   {"debug", s.debugCount},
                   {"info", s.infoCount},
                   {"warning", s.warningCount},
                   {"error", s.errorCount},
                   {"critical", s.criticalCount},
                   {"unknown", s.unknownCount}});
    out << ",\"entries\":[";
    bool first = true;
    for (const auto& e : file.parsed.entries) {
      if (!first)
        out << ',';
      first = false;
      emit(out, Json{{"timestamp", format_timestamp(e.timestamp)},
                     {"level", core::to_string(e.level)},
                     {"source", e.source.name},
                     {"message", e.message},
                     {"raw", e.rawContent},
                     {"attributes", e.attributes}});
    }
    out << "],\"malformed_records\":[";
    first = true;
    for (const auto& e : file.parsed.errors) {
      if (!first)
        out << ',';
      first = false;
      emit(out, Json{{"line", e.lineNumber}, {"raw", e.rawContent}, {"reason", e.message}});
    }
    out << "]}";
  }
  out << "]}\n";
  if (!out)
    throw std::runtime_error("unable to write report");
}

void save_json_report(const std::filesystem::path& destination,
                      std::span<const AnalysisResult> results) {
  if (std::filesystem::exists(destination)) {
    throw std::runtime_error("report destination already exists");
  }
  auto parent = destination.parent_path();
  if (parent.empty())
    parent = ".";
  std::random_device random;
  std::filesystem::path temp;
  for (int attempt = 0; attempt < 10; ++attempt) {
    temp = parent / (".loginsight-" + std::to_string(random()));
    if (std::filesystem::create_directory(temp))
      break;
    temp.clear();
  }
  if (temp.empty())
    throw std::runtime_error("unable to create temporary report directory");
  const TemporaryDirectory guard{temp};
  const auto staged = temp / "report.json";
  std::ofstream out{staged, std::ios::binary};
  write_json_report(out, results);
  out.close();
  if (!out)
    throw std::runtime_error("unable to close report");
  // Hard-link creation atomically fails if another process created the destination.
  std::filesystem::create_hard_link(staged, destination);
}
} // namespace loginsight::app
