#include "loginsight/app/analysis_summary_formatter.hpp"

#include <filesystem>

#include <gtest/gtest.h>

using loginsight::app::format_analysis_summary;
using loginsight::parsers::AnalysisSummary;

TEST(AnalysisSummaryFormatterTest, FormatsTheCompleteSummaryInCliOrder) {
  const AnalysisSummary summary{
      .validEntryCount = 196U,
      .malformedLineCount = 4U,
      .traceCount = 0U,
      .debugCount = 12U,
      .infoCount = 151U,
      .warningCount = 21U,
      .errorCount = 10U,
      .criticalCount = 2U,
      .unknownCount = 0U,
  };

  const auto formatted = format_analysis_summary("application.log", summary);

  EXPECT_EQ(formatted, "Analysis complete\n\n"
                       "File: application.log\n"
                       "Valid entries: 196\n"
                       "Malformed lines: 4\n\n"
                       "Trace: 0\n"
                       "Debug: 12\n"
                       "Info: 151\n"
                       "Warning: 21\n"
                       "Error: 10\n"
                       "Critical: 2\n"
                       "Unknown: 0\n");
}
