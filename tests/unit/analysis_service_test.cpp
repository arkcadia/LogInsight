#include "loginsight/app/analysis_service.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <sstream>

using namespace loginsight::app;
namespace {
std::filesystem::path fixture(const char* name) {
  return std::filesystem::path{LOGINSIGHT_TEST_FIXTURES_DIR} / name;
}
} // namespace
TEST(AnalysisServiceTest, ClassifiesFailuresAndPreservesPartialImports) {
  AnalysisService service;
  EXPECT_EQ(service.analyze(fixture("absent.log")).status, AnalysisStatus::file_not_found);
  EXPECT_EQ(service.analyze(fixture("unsupported.json")).status,
            AnalysisStatus::unsupported_format);
  EXPECT_EQ(service.analyze(fixture(".")).status, AnalysisStatus::io_failure);
  auto result = service.analyze(fixture("plain_text_mixed.log"));
  ASSERT_TRUE(result.completed());
  EXPECT_EQ(result.summary.validEntryCount, 2U);
  EXPECT_EQ(result.summary.malformedLineCount, 4U);
  EXPECT_EQ(result.parsed.entries[0].source.file_path, result.path.string());
}
TEST(AnalysisServiceTest, ImportsJsonLinesAndRetainsMalformedRecordLocations) {
  const auto result = AnalysisService{}.analyze(fixture("json_lines_mixed.ndjson"));
  ASSERT_TRUE(result.completed());
  ASSERT_EQ(result.parsed.entries.size(), 2U);
  ASSERT_EQ(result.parsed.errors.size(), 3U);
  EXPECT_EQ(result.summary.errorCount, 1U);
  EXPECT_EQ(result.parsed.entries.back().source.name, "worker");
  EXPECT_EQ(result.parsed.entries.back().message, "Failed");
  EXPECT_EQ(result.parsed.entries.back().source.file_path, result.path.string());
  EXPECT_EQ(result.parsed.errors[0].lineNumber, 2U);
  EXPECT_EQ(result.parsed.errors[1].rawContent, "{malformed");
  EXPECT_EQ(result.parsed.errors[2].lineNumber, 5U);
}
TEST(AnalysisServiceTest, ExportsEscapedEntriesAndMalformedRecords) {
  std::vector<AnalysisResult> files;
  files.push_back(AnalysisService{}.analyze(fixture("plain_text_mixed.log")));
  files[0].parsed.entries[0].message = "quote \" and newline\n";
  files[0].parsed.entries[1].message = std::string(1, static_cast<char>(0xff));
  std::ostringstream out;
  write_json_report(out, files);
  const auto report = nlohmann::json::parse(out.str());
  EXPECT_EQ(report["schema_version"], 1);
  EXPECT_EQ(report["files"][0]["entries"].size(), 2U);
  EXPECT_EQ(report["files"][0]["malformed_records"].size(), 4U);
  EXPECT_EQ(report["files"][0]["entries"][0]["message"], "quote \" and newline\n");
  EXPECT_EQ(report["files"][0]["entries"][1]["message"], "\xef\xbf\xbd");
  EXPECT_EQ(report["files"][0]["entries"][0]["timestamp"], "2026-07-13T18:42:10Z");
  std::ostringstream broken;
  broken.setstate(std::ios::badbit);
  EXPECT_THROW(write_json_report(broken, files), std::runtime_error);
}
TEST(AnalysisServiceTest, PublishesReportAndRefusesToOverwriteInput) {
  const auto path = std::filesystem::path{"service-report-test.json"};
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
  std::vector<AnalysisResult> files;
  files.push_back(AnalysisService{}.analyze(fixture("plain_text_valid.log")));
  save_json_report(path, files);
  std::ifstream input{path};
  const auto parsed = nlohmann::json::parse(input);
  EXPECT_EQ(parsed["files"][0]["summary"]["valid_entries"], 7);
  input.close();
  EXPECT_THROW(save_json_report(path, files), std::runtime_error);
  EXPECT_THROW(save_json_report(fixture("plain_text_valid.log"), files), std::runtime_error);
  std::filesystem::remove(path);
}
TEST(AnalysisServiceTest, FormatsPreEpochTimestampInUtc) {
  EXPECT_EQ(format_timestamp(loginsight::core::LogTimestamp{std::chrono::seconds{-1}}),
            "1969-12-31T23:59:59Z");
}

TEST(ImportBudgetTest, CountsBlankLongAndUnterminatedLinesAndChecksCancellation) {
  const std::string longLine(9000, 'x');
  std::istringstream input{"\n" + longLine + "\r\nlast"};
  loginsight::parsers::ImportBudget budget{};
  std::string line;
  ASSERT_TRUE(budget.readLine(input, line));
  EXPECT_TRUE(line.empty());
  ASSERT_TRUE(budget.readLine(input, line));
  EXPECT_EQ(line, longLine + "\r");
  ASSERT_TRUE(budget.readLine(input, line));
  EXPECT_EQ(line, "last");
  EXPECT_FALSE(budget.readLine(input, line));
  EXPECT_EQ(budget.lines, 3U);
  EXPECT_EQ(budget.bytes, longLine.size() + 7U);

  std::stop_source stop;
  budget.stop = stop.get_token();
  stop.request_stop();
  EXPECT_THROW(budget.readLine(input, line), std::runtime_error);
}

TEST(ImportBudgetTest, RejectsRecordsAndBatchesThatExceedLimits) {
  using loginsight::parsers::ImportBudget;
  std::string line;
  ImportBudget budget{};
  std::istringstream oversized{std::string(ImportBudget::maxLineBytes + 1, 'x')};
  EXPECT_THROW(budget.readLine(oversized, line), std::runtime_error);

  budget = ImportBudget{};
  budget.bytes = ImportBudget::maxBytes - 1;
  std::istringstream excessBytes{"x\n"};
  EXPECT_THROW(budget.readLine(excessBytes, line), std::runtime_error);

  budget = ImportBudget{};
  budget.lines = ImportBudget::maxLines;
  std::istringstream excessLine{"\n"};
  EXPECT_THROW(budget.readLine(excessLine, line), std::runtime_error);
}

TEST(AnalysisServiceTest, RedactsValuesAndOmitsRawTextFromBothReportScopes) {
  using nlohmann::json;
  std::array files{AnalysisService{}.analyze(fixture("plain_text_mixed.log"))};
  auto& file = files.front();
  file.path = "private-id.log";
  file.diagnostic = "details for private-id";
  auto& entry = file.parsed.entries.front();
  entry.message = "contact private-id twice private-id";
  entry.source.name = "private-id";
  entry.source.file_path = "/tmp/private-id.log";
  entry.attributes["trace_id"] = "private-id";
  entry.rawContent = R"(private-\u0069d)";
  file.parsed.errors.front().rawContent = "unlisted secret";
  const std::vector<std::string> terms{"private", "private-id"};
  std::ostringstream full;
  write_json_report(full, files, terms);
  const auto scrubbed = json::parse(full.str())["files"][0];
  EXPECT_EQ(scrubbed["path"], "[REDACTED].log");
  EXPECT_EQ(scrubbed["diagnostic"], "details for [REDACTED]");
  EXPECT_EQ(scrubbed["entries"][0]["raw"], "");
  EXPECT_EQ(scrubbed["malformed_records"][0]["raw"], "");

  const std::array<const loginsight::core::LogEntry*, 1> selected{&entry};
  std::ostringstream visible;
  write_visible_report(visible, selected, {{"trace_id", "private-id"}}, terms);
  const auto report = json::parse(visible.str());
  EXPECT_EQ(report["filters"]["trace_id"], "[REDACTED]");
  EXPECT_EQ(report["entries"][0]["path"], "/tmp/[REDACTED].log");
  EXPECT_EQ(report["entries"][0]["message"], "contact [REDACTED] twice [REDACTED]");
  EXPECT_EQ(entry.message, "contact private-id twice private-id");
}
