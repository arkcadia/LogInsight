#include "loginsight/parsers/analysis_summary.hpp"

#include "loginsight/core/log_entry.hpp"

#include <gtest/gtest.h>

using loginsight::core::LogEntry;
using loginsight::core::LogLevel;
using loginsight::parsers::build_analysis_summary;
using loginsight::parsers::ParseError;
using loginsight::parsers::ParseResult;

TEST(AnalysisSummaryTest, DefaultsAllCountsToZeroForAnEmptyParseResult) {
  const auto summary = build_analysis_summary(ParseResult{});

  EXPECT_EQ(summary.validEntryCount, 0U);
  EXPECT_EQ(summary.malformedLineCount, 0U);
  EXPECT_EQ(summary.traceCount, 0U);
  EXPECT_EQ(summary.debugCount, 0U);
  EXPECT_EQ(summary.infoCount, 0U);
  EXPECT_EQ(summary.warningCount, 0U);
  EXPECT_EQ(summary.errorCount, 0U);
  EXPECT_EQ(summary.criticalCount, 0U);
  EXPECT_EQ(summary.unknownCount, 0U);
}

TEST(AnalysisSummaryTest, CountsEveryLogLevelAndMalformedLines) {
  const ParseResult result{
      .entries =
          {
              LogEntry{.level = LogLevel::trace},
              LogEntry{.level = LogLevel::debug},
              LogEntry{.level = LogLevel::info},
              LogEntry{.level = LogLevel::warning},
              LogEntry{.level = LogLevel::warning},
              LogEntry{.level = LogLevel::error},
              LogEntry{.level = LogLevel::critical},
              LogEntry{.level = LogLevel::unknown},
          },
      .errors =
          {
              ParseError{.lineNumber = 2U, .rawContent = "invalid", .message = "invalid timestamp"},
              ParseError{.lineNumber = 5U, .rawContent = "invalid", .message = "missing level"},
          },
  };

  const auto summary = build_analysis_summary(result);

  EXPECT_EQ(summary.validEntryCount, 8U);
  EXPECT_EQ(summary.malformedLineCount, 2U);
  EXPECT_EQ(summary.traceCount, 1U);
  EXPECT_EQ(summary.debugCount, 1U);
  EXPECT_EQ(summary.infoCount, 1U);
  EXPECT_EQ(summary.warningCount, 2U);
  EXPECT_EQ(summary.errorCount, 1U);
  EXPECT_EQ(summary.criticalCount, 1U);
  EXPECT_EQ(summary.unknownCount, 1U);
}
