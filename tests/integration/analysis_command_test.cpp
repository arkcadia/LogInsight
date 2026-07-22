#include "loginsight/app/analysis_command.hpp"

#include "loginsight/parsers/file_import_service.hpp"
#include "loginsight/parsers/parser_registry.hpp"
#include "loginsight/parsers/plain_text_log_parser.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

using loginsight::app::CommandExitCode;
using loginsight::app::run_cli;
using loginsight::parsers::FileImportService;
using loginsight::parsers::ParserRegistry;
using loginsight::parsers::PlainTextLogParser;

namespace {

struct CommandOutput {
  int exitCode;
  std::string standardOutput;
  std::string standardError;
};

[[nodiscard]] std::filesystem::path fixture_path(const char* name) {
  return std::filesystem::path{LOGINSIGHT_TEST_FIXTURES_DIR} / name;
}

[[nodiscard]] CommandOutput run_command(std::span<const std::string_view> arguments,
                                        const FileImportService& import_service) {
  std::ostringstream standard_output;
  std::ostringstream standard_error;

  const auto exit_code = run_cli(arguments, import_service, standard_output, standard_error);
  return CommandOutput{
      .exitCode = exit_code,
      .standardOutput = standard_output.str(),
      .standardError = standard_error.str(),
  };
}

void register_plain_text_parser(ParserRegistry& registry) {
  ASSERT_TRUE(registry.registerParser(std::make_unique<PlainTextLogParser>()));
}

} // namespace

TEST(AnalysisCommandTest, SummarizesEntriesFromAPlainTextFixture) {
  ParserRegistry registry;
  register_plain_text_parser(registry);
  const FileImportService import_service{registry};
  const auto path = fixture_path("plain_text_valid.log").string();
  const std::array<std::string_view, 3U> arguments{"loginsight", "analyse", path};

  const auto result = run_command(arguments, import_service);

  EXPECT_EQ(result.exitCode, static_cast<int>(CommandExitCode::success));
  EXPECT_EQ(result.standardError, "");
  const auto expected_output = std::string{"Analysis complete\n\nFile: "} + path +
                               "\nValid entries: 7\nMalformed lines: 0\n\nTrace: 1\nDebug: 1\n"
                               "Info: 1\nWarning: 2\nError: 1\nCritical: 1\nUnknown: 0\n";
  EXPECT_EQ(result.standardOutput, expected_output);
}

TEST(AnalysisCommandTest, ReportsMissingFileArgument) {
  ParserRegistry registry;
  register_plain_text_parser(registry);
  const FileImportService import_service{registry};
  const std::array<std::string_view, 2U> arguments{"loginsight", "analyse"};

  const auto result = run_command(arguments, import_service);

  EXPECT_EQ(result.exitCode, static_cast<int>(CommandExitCode::invalid_arguments));
  EXPECT_EQ(result.standardOutput, "");
  EXPECT_EQ(result.standardError,
            "Error: missing file path.\nUsage: loginsight analyse <file-path>\n");
}

TEST(AnalysisCommandTest, ReportsMissingFiles) {
  ParserRegistry registry;
  register_plain_text_parser(registry);
  const FileImportService import_service{registry};
  const auto path = fixture_path("does_not_exist.log").string();
  const std::array<std::string_view, 3U> arguments{"loginsight", "analyse", path};

  const auto result = run_command(arguments, import_service);

  EXPECT_EQ(result.exitCode, static_cast<int>(CommandExitCode::file_not_found));
  EXPECT_EQ(result.standardOutput, "");
  EXPECT_EQ(result.standardError, std::string{"Error: file not found: "} + path + '\n');
}

TEST(AnalysisCommandTest, ReportsUnsupportedFormats) {
  ParserRegistry registry;
  register_plain_text_parser(registry);
  const FileImportService import_service{registry};
  const auto path = fixture_path("unsupported.json").string();
  const std::array<std::string_view, 3U> arguments{"loginsight", "analyse", path};

  const auto result = run_command(arguments, import_service);

  EXPECT_EQ(result.exitCode, static_cast<int>(CommandExitCode::unsupported_format));
  EXPECT_EQ(result.standardOutput, "");
  EXPECT_EQ(result.standardError, std::string{"Error: unsupported file format: "} + path + '\n');
}

TEST(AnalysisCommandTest, ReportsFileOpeningFailures) {
  ParserRegistry registry;
  register_plain_text_parser(registry);
  const FileImportService import_service{registry};
  const auto path = fixture_path(".").string();
  const std::array<std::string_view, 3U> arguments{"loginsight", "analyse", path};

  const auto result = run_command(arguments, import_service);

  EXPECT_EQ(result.exitCode, static_cast<int>(CommandExitCode::file_open_failure));
  EXPECT_EQ(result.standardOutput, "");
  EXPECT_EQ(result.standardError, std::string{"Error: unable to open file: "} + path + '\n');
}

TEST(AnalysisCommandTest, ReportsMalformedLinesWithoutFailingTheAnalysis) {
  ParserRegistry registry;
  register_plain_text_parser(registry);
  const FileImportService import_service{registry};
  const auto path = fixture_path("plain_text_mixed.log").string();
  const std::array<std::string_view, 3U> arguments{"loginsight", "analyse", path};

  const auto result = run_command(arguments, import_service);

  EXPECT_EQ(result.exitCode, static_cast<int>(CommandExitCode::success));
  EXPECT_EQ(result.standardError, "");
  const auto expected_output = std::string{"Analysis complete\n\nFile: "} + path +
                               "\nValid entries: 2\nMalformed lines: 4\n\nTrace: 0\nDebug: 0\n"
                               "Info: 1\nWarning: 0\nError: 1\nCritical: 0\nUnknown: 0\n";
  EXPECT_EQ(result.standardOutput, expected_output);
}
