#include "loginsight/parsers/plain_text_log_parser.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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

[[nodiscard]] std::ifstream open_fixture(const std::string& name) {
  const auto path = std::filesystem::path{LOGINSIGHT_TEST_FIXTURES_DIR} / name;
  std::ifstream file{path};
  EXPECT_TRUE(file.is_open()) << "Unable to open fixture: " << path;
  return file;
}

} // namespace

TEST(PlainTextLogParserTest, ParsesValidFixture) {
  auto file = open_fixture("plain_text_valid.log");
  const PlainTextLogParser parser;

  const auto result = parser.parse(file, "plain_text_valid.log");

  ASSERT_FALSE(result.has_errors());
  ASSERT_EQ(result.entries.size(), 7U);

  EXPECT_EQ(result.entries[0].timestamp, timestamp(2026, 7U, 12U, 15, 21, 4));
  EXPECT_EQ(result.entries[0].level, LogLevel::trace);
  EXPECT_EQ(result.entries[0].message, "Trace details enabled");
  EXPECT_EQ(result.entries[0].rawContent, "2026-07-12 15:21:04 TRACE Trace details enabled");
  EXPECT_EQ(result.entries[0].source.name, "plain_text_valid.log");

  EXPECT_EQ(result.entries[1].level, LogLevel::debug);
  EXPECT_EQ(result.entries[2].level, LogLevel::info);
  EXPECT_EQ(result.entries[3].level, LogLevel::warning);
  EXPECT_EQ(result.entries[4].level, LogLevel::warning);
  EXPECT_EQ(result.entries[5].level, LogLevel::error);
  EXPECT_EQ(result.entries[6].level, LogLevel::critical);
  EXPECT_EQ(result.entries[6].message, "Disk is full");
}

TEST(PlainTextLogParserTest, KeepsImportingAfterMalformedLines) {
  auto file = open_fixture("plain_text_mixed.log");
  const PlainTextLogParser parser;

  const auto result = parser.parse(file, "plain_text_mixed.log");

  ASSERT_EQ(result.entries.size(), 2U);
  ASSERT_EQ(result.errors.size(), 4U);

  EXPECT_EQ(result.entries[0].level, LogLevel::info);
  EXPECT_EQ(result.entries[0].message, "Import started");
  EXPECT_EQ(result.entries[0].rawContent, "2026-07-12 15:21:04 INFO Import started");

  EXPECT_EQ(result.entries[1].level, LogLevel::error);
  EXPECT_EQ(result.entries[1].message, "Import finished with issues");
  EXPECT_EQ(result.entries[1].rawContent, "2026-07-12 15:25:10 ERROR Import finished with issues");

  EXPECT_EQ(result.errors[0].line_number, 2U);
  EXPECT_EQ(result.errors[0].line, "not a valid log line");
  EXPECT_EQ(result.errors[0].message, "line is too short for the expected plain text format");

  EXPECT_EQ(result.errors[1].line_number, 3U);
  EXPECT_EQ(result.errors[1].line, "2026-99-12 15:21:04 WARN Invalid month");
  EXPECT_EQ(result.errors[1].message, "invalid timestamp");

  EXPECT_EQ(result.errors[2].line_number, 4U);
  EXPECT_EQ(result.errors[2].line, "2026-07-12 15:21:04 NOTICE Unsupported level");
  EXPECT_EQ(result.errors[2].message, "unsupported log level");

  EXPECT_EQ(result.errors[3].line_number, 5U);
  EXPECT_EQ(result.errors[3].line, "2026-07-12 15:21:04 ERROR");
  EXPECT_EQ(result.errors[3].message, "missing message");
}

TEST(PlainTextLogParserTest, ParsesWarnAsWarning) {
  std::istringstream input{"2026-07-12 15:21:04 WARN Slow query detected\n"};
  const PlainTextLogParser parser;

  const auto result = parser.parse(input);

  ASSERT_TRUE(result.errors.empty());
  ASSERT_EQ(result.entries.size(), 1U);
  EXPECT_EQ(result.entries.front().level, LogLevel::warning);
  EXPECT_EQ(result.entries.front().message, "Slow query detected");
}

TEST(PlainTextLogParserTest, RejectsInvalidDateAndTimeValues) {
  std::istringstream input{"2026-02-30 15:21:04 ERROR Impossible date\n"
                           "2026-07-12 25:21:04 ERROR Impossible hour\n"
                           "2026-07-12 15:61:04 ERROR Impossible minute\n"
                           "2026-07-12 15:21:61 ERROR Impossible second\n"};
  const PlainTextLogParser parser;

  const auto result = parser.parse(input);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 4U);
  EXPECT_EQ(result.errors[0].message, "invalid timestamp");
  EXPECT_EQ(result.errors[1].message, "invalid timestamp");
  EXPECT_EQ(result.errors[2].message, "invalid timestamp");
  EXPECT_EQ(result.errors[3].message, "invalid timestamp");
}
