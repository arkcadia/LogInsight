#include "loginsight/parsers/parse_result.hpp"

#include "loginsight/core/log_entry.hpp"

#include <gtest/gtest.h>

using loginsight::core::LogEntry;
using loginsight::core::LogLevel;
using loginsight::parsers::ParseError;
using loginsight::parsers::ParseResult;

TEST(ParseErrorTest, DefaultsToSafeEmptyValues) {
  const ParseError error;

  EXPECT_EQ(error.lineNumber, 0U);
  EXPECT_TRUE(error.rawContent.empty());
  EXPECT_TRUE(error.message.empty());
}

TEST(ParseResultTest, DefaultsToNoEntriesAndNoErrors) {
  const ParseResult result;

  EXPECT_TRUE(result.entries.empty());
  EXPECT_TRUE(result.errors.empty());
  EXPECT_FALSE(result.has_errors());
}

TEST(ParseResultTest, StoresEntriesAndErrorsByValue) {
  const ParseResult result{
      .entries = {LogEntry{
          .level = LogLevel::error,
          .message = "Database connection failed",
          .rawContent = "2026-07-12 15:21:04 ERROR Database connection failed",
      }},
      .errors = {ParseError{
          .lineNumber = 2U,
          .rawContent = "malformed line",
          .message = "invalid timestamp",
      }},
  };

  ASSERT_EQ(result.entries.size(), 1U);
  ASSERT_EQ(result.errors.size(), 1U);
  EXPECT_TRUE(result.has_errors());
  EXPECT_EQ(result.entries.front().message, "Database connection failed");
  EXPECT_EQ(result.errors.front().lineNumber, 2U);
  EXPECT_EQ(result.errors.front().rawContent, "malformed line");
}
