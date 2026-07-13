#include "loginsight/parsers/plain_text_log_parser.hpp"

#include <chrono>
#include <filesystem>

#include <gtest/gtest.h>

using loginsight::core::LogLevel;
using loginsight::core::LogTimestamp;
using loginsight::parsers::PlainTextLogParser;

namespace {

[[nodiscard]] std::filesystem::path fixture_path(const char* name) {
  return std::filesystem::path{LOGINSIGHT_TEST_FIXTURES_DIR} / name;
}

[[nodiscard]] LogTimestamp timestamp(int year, unsigned month, unsigned day, int hour, int minute,
                                     int second) {
  const auto date = std::chrono::year{year} / std::chrono::month{month} / std::chrono::day{day};
  const auto value = std::chrono::sys_days{date} + std::chrono::hours{hour} +
                     std::chrono::minutes{minute} + std::chrono::seconds{second};
  return std::chrono::time_point_cast<LogTimestamp::duration>(value);
}

} // namespace

TEST(PlainTextLogParserTest, ParsesCompletelyValidFixture) {
  const PlainTextLogParser parser;

  const auto result = parser.parse(fixture_path("plain_text_valid.log"));

  ASSERT_FALSE(result.has_errors());
  ASSERT_EQ(result.entries.size(), 7U);

  EXPECT_EQ(result.entries[0].timestamp, timestamp(2026, 7U, 13U, 18, 42, 10));
  EXPECT_EQ(result.entries[0].level, LogLevel::trace);
  EXPECT_EQ(result.entries[0].source.name, "gateway");
  EXPECT_EQ(result.entries[0].message, "Trace details enabled for request id=42");

  EXPECT_EQ(result.entries[1].level, LogLevel::debug);
  EXPECT_EQ(result.entries[2].level, LogLevel::info);
  EXPECT_EQ(result.entries[3].level, LogLevel::warning);
  EXPECT_EQ(result.entries[4].level, LogLevel::warning);
  EXPECT_EQ(result.entries[5].level, LogLevel::error);
  EXPECT_EQ(result.entries[6].level, LogLevel::critical);
  EXPECT_EQ(result.entries[6].message, "Disk /var/log is 99% full!");
}

TEST(PlainTextLogParserTest, KeepsValidEntriesAndMalformedErrorsFromMixedFixture) {
  const PlainTextLogParser parser;

  const auto result = parser.parse(fixture_path("plain_text_mixed.log"));

  ASSERT_EQ(result.entries.size(), 2U);
  ASSERT_EQ(result.errors.size(), 4U);

  EXPECT_EQ(result.entries[0].level, LogLevel::info);
  EXPECT_EQ(result.entries[0].source.name, "gateway");
  EXPECT_EQ(result.entries[0].message, "Import started: batch=17");
  EXPECT_EQ(result.entries[0].rawContent,
            "2026-07-13 18:42:10 INFO gateway Import started: batch=17");

  EXPECT_EQ(result.entries[1].level, LogLevel::error);
  EXPECT_EQ(result.entries[1].source.name, "payment-service");
  EXPECT_EQ(result.entries[1].message, "Import finished with 2 errors");

  EXPECT_EQ(result.errors[0].lineNumber, 2U);
  EXPECT_EQ(result.errors[0].rawContent,
            "2026-99-13 18:42:11 WARN worker Invalid month should fail");
  EXPECT_EQ(result.errors[0].message, "invalid timestamp");

  EXPECT_EQ(result.errors[1].lineNumber, 4U);
  EXPECT_EQ(result.errors[1].rawContent, "2026-07-13 18:42:12 NOTICE gateway Unsupported level");
  EXPECT_EQ(result.errors[1].message, "unsupported log level");

  EXPECT_EQ(result.errors[2].lineNumber, 5U);
  EXPECT_EQ(result.errors[2].rawContent, "2026-07-13 18:42:13 ERROR");
  EXPECT_EQ(result.errors[2].message, "missing source");

  EXPECT_EQ(result.errors[3].lineNumber, 6U);
  EXPECT_EQ(result.errors[3].rawContent, "2026-07-13 18:42:14 WARN worker");
  EXPECT_EQ(result.errors[3].message, "missing message");
}

TEST(PlainTextLogParserTest, ReportsAllLinesFromMalformedFixture) {
  const PlainTextLogParser parser;

  const auto result = parser.parse(fixture_path("plain_text_malformed.log"));

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 7U);
  EXPECT_EQ(result.errors[0].lineNumber, 1U);
  EXPECT_EQ(result.errors[0].rawContent, "not a log line");
  EXPECT_EQ(result.errors[0].message, "invalid timestamp");
  EXPECT_EQ(result.errors[1].message, "invalid timestamp");
  EXPECT_EQ(result.errors[2].message, "invalid timestamp");
  EXPECT_EQ(result.errors[3].message, "invalid timestamp");
  EXPECT_EQ(result.errors[4].message, "missing level");
  EXPECT_EQ(result.errors[5].message, "missing source");
  EXPECT_EQ(result.errors[6].message, "unsupported log level");
}

TEST(PlainTextLogParserTest, ParsesEmptyFixture) {
  const PlainTextLogParser parser;

  const auto result = parser.parse(fixture_path("plain_text_empty.log"));

  EXPECT_TRUE(result.entries.empty());
  EXPECT_TRUE(result.errors.empty());
}

TEST(PlainTextLogParserTest, HandlesAdditionalSpacesAndPunctuation) {
  const PlainTextLogParser parser;

  const auto result = parser.parse(fixture_path("plain_text_spacing.log"));

  ASSERT_TRUE(result.errors.empty());
  ASSERT_EQ(result.entries.size(), 2U);
  EXPECT_EQ(result.entries[0].level, LogLevel::info);
  EXPECT_EQ(result.entries[0].source.name, "api-gateway");
  EXPECT_EQ(result.entries[0].message, "Request completed: status=200 path=/checkout");
  EXPECT_EQ(result.entries[0].rawContent,
            "2026-07-13 18:42:10   INFO   api-gateway   Request completed: status=200 "
            "path=/checkout");

  EXPECT_EQ(result.entries[1].level, LogLevel::warning);
  EXPECT_EQ(result.entries[1].source.name, "worker-1");
  EXPECT_EQ(result.entries[1].message, "Queue depth is 42; threshold=30.");
}

TEST(PlainTextLogParserTest, ReportsFileOpenFailure) {
  const auto path = fixture_path("does_not_exist.log");
  const PlainTextLogParser parser;

  const auto result = parser.parse(path);

  EXPECT_TRUE(result.entries.empty());
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_EQ(result.errors.front().lineNumber, 0U);
  EXPECT_EQ(result.errors.front().rawContent, path.string());
  EXPECT_EQ(result.errors.front().message, "unable to open file");
}

TEST(PlainTextLogParserTest, DetectsSupportedFileExtensions) {
  const PlainTextLogParser parser;

  EXPECT_TRUE(parser.canParse("application.log"));
  EXPECT_TRUE(parser.canParse("application.txt"));
  EXPECT_TRUE(parser.canParse("APPLICATION.LOG"));
  EXPECT_FALSE(parser.canParse("application.json"));
}
