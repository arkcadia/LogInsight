#include "loginsight/app/analysis_service.hpp"

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
AnalysisService::AnalysisService(parsers::JsonParserConfig config, parsers::CsvColumns csv)
    : registry_(parsers::default_parser_registry(std::move(config), std::move(csv))) {}

AnalysisResult AnalysisService::analyze(const std::filesystem::path& path,
                                        parsers::ImportBudget* budget) const {
  return analyze_file(path, parsers::FileImportService{registry_}, budget);
}

AnalysisResult analyze_file(const std::filesystem::path& path,
                            const parsers::FileImportService& importer,
                            parsers::ImportBudget* budget) {
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
    auto imported = importer.importFile(path, budget);
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
      << ':' << std::setw(2) << time.seconds().count();
  using namespace std::chrono;
  auto fraction = duration_cast<nanoseconds>(timestamp.time_since_epoch() % seconds{1});
  if (fraction < nanoseconds::zero())
    fraction += seconds{1};
  if (fraction != nanoseconds::zero()) {
    auto digits = std::to_string((seconds{1} + fraction).count()).substr(1);
    while (digits.back() == '0')
      digits.pop_back();
    out << '.' << digits;
  }
  out << 'Z';
  return out.str();
}

namespace {
using Json = nlohmann::json;
void redact_values(Json& value, std::span<const std::string> terms) {
  if (value.is_structured()) {
    for (auto& child : value)
      redact_values(child, terms);
    return;
  }
  auto* field = value.get_ptr<std::string*>();
  if (!field)
    return;
  std::string_view remaining{*field};
  std::string masked;
  for (;;) {
    auto position = std::string_view::npos;
    std::size_t length{};
    for (const auto& term : terms) {
      if (term.empty())
        continue;
      const auto found = remaining.find(term);
      if (found < position ||
          (found == position && found != std::string_view::npos && term.size() > length)) {
        position = found;
        length = term.size();
      }
    }
    if (position == std::string_view::npos)
      break;
    masked.append(remaining.substr(0, position));
    masked += "[REDACTED]";
    remaining.remove_prefix(position + length);
  }
  if (!masked.empty())
    *field = masked.append(remaining);
}

void emit(std::ostream& out, Json value, std::span<const std::string> redact) {
  if (!redact.empty())
    redact_values(value, redact);
  // Display/export invalid UTF-8 as replacement characters rather than aborting an import.
  out << value.dump(-1, ' ', false, Json::error_handler_t::replace);
}
Json entry_json(const core::LogEntry& entry, bool redact) {
  return {{"timestamp", format_timestamp(entry.timestamp)},
          {"level", core::to_string(entry.level)},
          {"source", entry.source.name},
          {"message", entry.message},
          {"raw", redact ? std::string{} : entry.rawContent},
          {"attributes", entry.attributes}};
}

struct TemporaryDirectory {
  std::filesystem::path path;
  ~TemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};
} // namespace

void write_json_report(std::ostream& out, std::span<const AnalysisResult> results,
                       std::span<const std::string> redact) {
  out << "{\"schema_version\":1,\"files\":[";
  bool first_file = true;
  for (const auto& file : results) {
    if (!first_file)
      out << ',';
    first_file = false;
    out << "{\"path\":";
    emit(out, file.path.string(), redact);
    out << ",\"completed\":" << (file.completed() ? "true" : "false") << ",\"diagnostic\":";
    emit(out, file.diagnostic, redact);
    const auto& s = file.summary;
    out << ",\"summary\":";
    emit(out,
         Json{{"valid_entries", s.validEntryCount},
              {"malformed_records", s.malformedLineCount},
              {"trace", s.traceCount},
              {"debug", s.debugCount},
              {"info", s.infoCount},
              {"warning", s.warningCount},
              {"error", s.errorCount},
              {"critical", s.criticalCount},
              {"unknown", s.unknownCount}},
         redact);
    out << ",\"entries\":[";
    bool first = true;
    for (const auto& e : file.parsed.entries) {
      if (!first)
        out << ',';
      first = false;
      emit(out, entry_json(e, !redact.empty()), redact);
    }
    out << "],\"malformed_records\":[";
    first = true;
    for (const auto& e : file.parsed.errors) {
      if (!first)
        out << ',';
      first = false;
      emit(out,
           Json{{"line", e.lineNumber},
                {"raw", redact.empty() ? e.rawContent : std::string{}},
                {"reason", e.message}},
           redact);
    }
    out << "]}";
  }
  out << "]}\n";
  if (!out)
    throw std::runtime_error("unable to write report");
}

namespace {
void save_report(const std::filesystem::path& destination, const auto& write) {
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
  write(out);
  out.close();
  if (!out)
    throw std::runtime_error("unable to close report");
  // Hard-link creation atomically fails if another process created the destination.
  std::filesystem::create_hard_link(staged, destination);
}
} // namespace

void save_json_report(const std::filesystem::path& destination,
                      std::span<const AnalysisResult> results,
                      std::span<const std::string> redact) {
  save_report(destination, [&](auto& out) { write_json_report(out, results, redact); });
}

void write_visible_report(std::ostream& out, std::span<const core::LogEntry* const> entries,
                          const core::LogAttributes& filters, std::span<const std::string> redact) {
  out << "{\"schema_version\":1,\"scope\":\"visible\",\"filters\":";
  emit(out, filters, redact);
  out << ",\"entries\":[";
  bool first = true;
  for (const auto* entry : entries) {
    if (!first)
      out << ',';
    first = false;
    auto value = entry_json(*entry, !redact.empty());
    value["path"] = entry->source.file_path.value_or("");
    emit(out, std::move(value), redact);
  }
  out << "]}\n";
  if (!out)
    throw std::runtime_error("unable to write report");
}

void save_visible_report(const std::filesystem::path& destination,
                         std::span<const core::LogEntry* const> entries,
                         const core::LogAttributes& filters, std::span<const std::string> redact) {
  save_report(destination, [&](auto& out) { write_visible_report(out, entries, filters, redact); });
}
} // namespace loginsight::app
