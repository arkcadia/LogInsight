#include "loginsight/app/analysis_command.hpp"
#include "loginsight/app/analysis_service.hpp"
#include "loginsight/app/analysis_summary_formatter.hpp"

#include <array>
#include <exception>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>

namespace loginsight::app {
namespace {
constexpr std::string_view usage = "Usage: loginsight analyse <file-path>\n";
int code(CommandExitCode value) { return static_cast<int>(value); }
int usage_error(std::ostream& out, std::string_view message) {
  out << "Error: " << message << '\n' << usage;
  return code(CommandExitCode::invalid_arguments);
}
} // namespace

int run_cli(std::span<const std::string_view> arguments,
            const parsers::FileImportService& import_service, std::ostream& output,
            std::ostream& error_output) {
  if ((arguments.size() == 2U && (arguments[1] == "--help" || arguments[1] == "-h")) ||
      (arguments.size() == 3U && arguments[1] == "analyse" && arguments[2] == "--help")) {
    output << "LogInsight — local log analysis\n\n"
           << usage << "       loginsight analyse <file-path> --json <report-path>\n\n"
           << "Supported: .log and .txt, one record per line:\n"
           << "YYYY-MM-DD HH:MM:SS LEVEL SOURCE MESSAGE\n\n"
           << "JSON Lines (.jsonl, .ndjson): timestamp, level, source, message.\n"
           << "CSV (.csv): header with timestamp, level, source, message columns.\n"
           << "Map columns with --csv-columns <timestamp> <level> <source> <message>.\n"
           << "Pass an empty source argument to use the filename stem.\n"
           << "Malformed records are counted; valid records remain available.\n"
           << "JSON export includes entries and malformed records; destination must not exist.\n"
           << "Example: loginsight analyse examples/logs/atlas.log --json report.json\n"
           << "Exit codes: 0 success (including malformed records), 64 arguments,\n"
           << "65 unsupported format, 66 missing file, 74 I/O, 78 configuration.\n";
    output.flush();
    return code(output ? CommandExitCode::success : CommandExitCode::file_open_failure);
  }
  if (arguments.size() < 2U)
    return usage_error(error_output, "missing command.");
  if (arguments[1] != "analyse") {
    return usage_error(error_output, "unsupported command: " + std::string{arguments[1]});
  }
  if (arguments.size() < 3U || arguments[2].empty()) {
    return usage_error(error_output, "missing file path.");
  }
  std::optional<std::string_view> report;
  std::optional<parsers::CsvColumns> csv;
  for (std::size_t i = 3; i < arguments.size();) {
    if (arguments[i] == "--json" && !report && i + 1 < arguments.size() &&
        !arguments[i + 1].empty()) {
      report = arguments[i + 1];
      i += 2;
    } else if (arguments[i] == "--csv-columns" && !csv && i + 4 < arguments.size()) {
      csv = parsers::CsvColumns{std::string{arguments[i + 1]}, std::string{arguments[i + 2]},
                                std::string{arguments[i + 3]}, std::string{arguments[i + 4]}};
      if (!csv->valid() || !parsers::CsvLogParser{}.canParse(arguments[2]))
        return usage_error(error_output, "invalid CSV column mappings or input extension.");
      i += 5;
    } else {
      return usage_error(error_output,
                         "expected a file path and optional --json or --csv-columns.");
    }
  }
  try {
    const std::filesystem::path path{arguments[2]};
    auto result =
        csv ? AnalysisService{{}, *csv}.analyze(path) : analyze_file(path, import_service);
    if (!result.completed()) {
      auto exit = CommandExitCode::file_open_failure;
      auto diagnostic = result.diagnostic;
      if (result.status == AnalysisStatus::unsupported_format)
        exit = CommandExitCode::unsupported_format;
      if (result.status == AnalysisStatus::file_not_found)
        exit = CommandExitCode::file_not_found;
      if (result.status == AnalysisStatus::io_failure)
        diagnostic = "unable to open file";
      error_output << "Error: " << diagnostic << ": " << result.path.string() << '\n';
      return code(exit);
    }
    output << format_analysis_summary(result.path, result.summary);
    if (report) {
      const std::array<AnalysisResult, 1> results{std::move(result)};
      save_json_report(std::filesystem::path{*report}, results);
      output << "Report: " << *report << '\n';
    }
    output.flush();
    if (!output)
      throw std::runtime_error("unable to write command output");
    return code(CommandExitCode::success);
  } catch (const std::exception& error) {
    error_output << "Error: " << error.what() << '\n';
    return code(CommandExitCode::file_open_failure);
  }
}
} // namespace loginsight::app
