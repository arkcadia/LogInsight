#include "loginsight/app/analysis_command.hpp"

#include "loginsight/app/analysis_summary_formatter.hpp"

#include <algorithm>
#include <filesystem>
#include <ostream>
#include <string>
#include <system_error>

namespace loginsight::app {
namespace {

constexpr std::string_view analyse_command = "analyse";
constexpr std::string_view usage = "Usage: loginsight analyse <file-path>\n";

[[nodiscard]] int exit_code(CommandExitCode code) noexcept { return static_cast<int>(code); }

[[nodiscard]] bool has_file_opening_failure(const parsers::ParseResult& result) {
  return std::any_of(result.errors.cbegin(), result.errors.cend(),
                     [](const parsers::ParseError& error) { return error.lineNumber == 0U; });
}

void print_usage_error(std::ostream& error_output, std::string_view message) {
  error_output << "Error: " << message << '\n' << usage;
}

} // namespace

int run_cli(std::span<const std::string_view> arguments,
            const parsers::FileImportService& import_service, std::ostream& output,
            std::ostream& error_output) {
  if (arguments.size() < 2U) {
    print_usage_error(error_output, "missing command.");
    return exit_code(CommandExitCode::invalid_arguments);
  }

  if (arguments[1] != analyse_command) {
    print_usage_error(error_output, "unsupported command: " + std::string{arguments[1]});
    return exit_code(CommandExitCode::invalid_arguments);
  }

  if (arguments.size() < 3U || arguments[2].empty()) {
    print_usage_error(error_output, "missing file path.");
    return exit_code(CommandExitCode::invalid_arguments);
  }

  if (arguments.size() > 3U) {
    print_usage_error(error_output, "expected exactly one file path.");
    return exit_code(CommandExitCode::invalid_arguments);
  }

  const std::filesystem::path path{std::string{arguments[2]}};
  std::error_code status_error;
  const auto file_status = std::filesystem::status(path, status_error);
  if (status_error == std::errc::no_such_file_or_directory) {
    error_output << "Error: file not found: " << path.string() << '\n';
    return exit_code(CommandExitCode::file_not_found);
  }

  if (!status_error && !std::filesystem::exists(file_status)) {
    error_output << "Error: file not found: " << path.string() << '\n';
    return exit_code(CommandExitCode::file_not_found);
  }

  if (status_error || !std::filesystem::is_regular_file(file_status)) {
    error_output << "Error: unable to open file: " << path.string() << '\n';
    return exit_code(CommandExitCode::file_open_failure);
  }

  const auto import_result = import_service.importFile(path);
  if (!import_result.isSupported()) {
    error_output << "Error: unsupported file format: " << path.string() << '\n';
    return exit_code(CommandExitCode::unsupported_format);
  }

  if (has_file_opening_failure(import_result.parseResult)) {
    error_output << "Error: unable to open file: " << path.string() << '\n';
    return exit_code(CommandExitCode::file_open_failure);
  }

  const auto summary = parsers::build_analysis_summary(import_result.parseResult);

  output << format_analysis_summary(path, summary);

  return exit_code(CommandExitCode::success);
}

} // namespace loginsight::app
