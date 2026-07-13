#include "loginsight/parsers/plain_text_log_parser.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

using loginsight::core::LogLevel;
using loginsight::core::LogTimestamp;
using loginsight::parsers::PlainTextLogParser;

namespace {

[[nodiscard]] LogTimestamp timestamp(int year, unsigned month, unsigned day, int hour, int minute,
                                     int second) {
  const auto date = std::chrono::year{year} / std::chrono::month{month} / std::chrono::day{day};
  const auto value = std::chrono::sys_days{date} + std::chrono::hours{hour} +
                     std::chrono::minutes{minute} + std::chrono::seconds{second};
  return std::chrono::time_point_cast<LogTimestamp::duration>(value);
}

[[nodiscard]] std::filesystem::path write_log_file(std::string_view name,
                                                   std::string_view content) {
  const auto path = std::filesystem::temp_directory_path() / std::string{name};
  std::ofstream file{path};
  EXPECT_TRUE(file.is_open()) << "Unable to write test log file: " << path;
  file << content;
  return path;
}

} // namespace

TEST(PlainTextLogParserTest, ParsesOneValidLine) {
  const auto raw_line = "2026-07-13 18:42:10 ERROR payment-service Payment processing failed";
  const auto path = write_log_file("loginsight_plain_text_one_valid.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  ASSERT_FALSE(result.has_errors());
  ASSERT_EQ(result.entries.size(), 1U);
  const auto& entry = result.entries.front();
  EXPECT_EQ(entry.timestamp, timestamp(2026, 7U, 13U, 18, 42, 10));
  EXPECT_EQ(entry.level, LogLevel::error);
  EXPECT_EQ(entry.source.name, "payment-service");
  EXPECT_EQ(entry.message, "Payment processing failed");
  EXPECT_EQ(entry.rawContent, raw_line);
}

TEST(PlainTextLogParserTest, ParsesMultipleValidLines) {
  const auto path = write_log_file("loginsight_plain_text_multiple_valid.log",
                                   "2026-07-13 18:42:10 INFO gateway Request accepted\n"
                                   "2026-07-13 18:42:11 warn worker Retry scheduled\n"
                                   "2026-07-13 18:42:12 CRITICAL database Storage unavailable\n");
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  ASSERT_FALSE(result.has_errors());
  ASSERT_EQ(result.entries.size(), 3U);
  EXPECT_EQ(result.entries[0].level, LogLevel::info);
  EXPECT_EQ(result.entries[0].source.name, "gateway");
  EXPECT_EQ(result.entries[1].level, LogLevel::warning);
  EXPECT_EQ(result.entries[1].source.name, "worker");
  EXPECT_EQ(result.entries[2].level, LogLevel::critical);
  EXPECT_EQ(result.entries[2].source.name, "database");
}

TEST(PlainTextLogParserTest, ParsesEmptyFile) {
  const auto path = write_log_file("loginsight_plain_text_empty.log", "");
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  EXPECT_TRUE(result.errors.empty());
}

TEST(PlainTextLogParserTest, ReportsFileOpenFailure) {
  const auto path = std::filesystem::temp_directory_path() / "loginsight_missing_plain_text.log";
  std::filesystem::remove(path);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_EQ(result.errors.front().lineNumber, 0U);
  EXPECT_EQ(result.errors.front().rawContent, path.string());
  EXPECT_EQ(result.errors.front().message, "unable to open file");
}

TEST(PlainTextLogParserTest, ReportsInvalidTimestamp) {
  const auto raw_line = "2026-99-13 18:42:10 ERROR payment-service Invalid month";
  const auto path = write_log_file("loginsight_plain_text_invalid_timestamp.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_EQ(result.errors.front().lineNumber, 1U);
  EXPECT_EQ(result.errors.front().rawContent, raw_line);
  EXPECT_EQ(result.errors.front().message, "invalid timestamp");
}

TEST(PlainTextLogParserTest, ReportsMissingLevel) {
  const auto raw_line = "2026-07-13 18:42:10   ";
  const auto path = write_log_file("loginsight_plain_text_missing_level.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_EQ(result.errors.front().message, "missing level");
  EXPECT_EQ(result.errors.front().rawContent, raw_line);
}

TEST(PlainTextLogParserTest, ReportsInvalidLevelAndRejectsUnknown) {
  const auto raw_line = "2026-07-13 18:42:10 UNKNOWN payment-service Not classified";
  const auto path = write_log_file("loginsight_plain_text_invalid_level.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_EQ(result.errors.front().message, "unsupported log level");
  EXPECT_EQ(result.errors.front().rawContent, raw_line);
}

TEST(PlainTextLogParserTest, ReportsMissingSource) {
  const auto raw_line = "2026-07-13 18:42:10 ERROR";
  const auto path = write_log_file("loginsight_plain_text_missing_source.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_EQ(result.errors.front().message, "missing source");
  EXPECT_EQ(result.errors.front().rawContent, raw_line);
}

TEST(PlainTextLogParserTest, ReportsMissingMessage) {
  const auto raw_line = "2026-07-13 18:42:10 ERROR payment-service";
  const auto path = write_log_file("loginsight_plain_text_missing_message.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_EQ(result.errors.front().message, "missing message");
  EXPECT_EQ(result.errors.front().rawContent, raw_line);
}

TEST(PlainTextLogParserTest, AllowsExtraSpacesBetweenFields) {
  const auto raw_line = "2026-07-13 18:42:10   error   payment-service   Payment processing failed";
  const auto path = write_log_file("loginsight_plain_text_extra_spaces.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  ASSERT_TRUE(result.errors.empty());
  ASSERT_EQ(result.entries.size(), 1U);
  EXPECT_EQ(result.entries.front().level, LogLevel::error);
  EXPECT_EQ(result.entries.front().source.name, "payment-service");
  EXPECT_EQ(result.entries.front().message, "Payment processing failed");
  EXPECT_EQ(result.entries.front().rawContent, raw_line);
}

TEST(PlainTextLogParserTest, PreservesMessageContainingSpaces) {
  const auto raw_line =
      "2026-07-13 18:42:10 WARNING payment-service Payment failed after 3 attempts";
  const auto path = write_log_file("loginsight_plain_text_message_spaces.log", raw_line);
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  ASSERT_TRUE(result.errors.empty());
  ASSERT_EQ(result.entries.size(), 1U);
  EXPECT_EQ(result.entries.front().message, "Payment failed after 3 attempts");
}

TEST(PlainTextLogParserTest, KeepsValidEntriesAndMalformedErrors) {
  const auto path =
      write_log_file("loginsight_plain_text_mixed.log",
                     "2026-07-13 18:42:10 INFO gateway Import started\n"
                     "2026-07-13 18:42:11 NOTICE gateway Unsupported level\n"
                     "\n"
                     "2026-07-13 18:42:12 ERROR payment-service\n"
                     "2026-07-13 18:42:13 WARN worker Import completed with warnings\n");
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  ASSERT_EQ(result.entries.size(), 2U);
  ASSERT_EQ(result.errors.size(), 2U);
  EXPECT_EQ(result.entries[0].source.name, "gateway");
  EXPECT_EQ(result.entries[0].message, "Import started");
  EXPECT_EQ(result.entries[1].level, LogLevel::warning);
  EXPECT_EQ(result.entries[1].source.name, "worker");

  EXPECT_EQ(result.errors[0].lineNumber, 2U);
  EXPECT_EQ(result.errors[0].rawContent, "2026-07-13 18:42:11 NOTICE gateway Unsupported level");
  EXPECT_EQ(result.errors[0].message, "unsupported log level");

  EXPECT_EQ(result.errors[1].lineNumber, 4U);
  EXPECT_EQ(result.errors[1].rawContent, "2026-07-13 18:42:12 ERROR payment-service");
  EXPECT_EQ(result.errors[1].message, "missing message");
}

TEST(PlainTextLogParserTest, DetectsSupportedFileExtensions) {
  const PlainTextLogParser parser;

  EXPECT_TRUE(parser.canParse("application.log"));
  EXPECT_TRUE(parser.canParse("application.txt"));
  EXPECT_TRUE(parser.canParse("APPLICATION.LOG"));
  EXPECT_FALSE(parser.canParse("application.json"));
}
