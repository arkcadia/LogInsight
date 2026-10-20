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
