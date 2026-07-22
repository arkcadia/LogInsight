#include "loginsight/parsers/analysis_summary.hpp"

#include "loginsight/core/log_level.hpp"

namespace loginsight::parsers {

AnalysisSummary build_analysis_summary(const ParseResult& result) noexcept {
  AnalysisSummary summary{
      .validEntryCount = result.entries.size(),
      .malformedLineCount = result.errors.size(),
  };

  for (const auto& entry : result.entries) {
    switch (entry.level) {
    case core::LogLevel::trace:
      ++summary.traceCount;
      break;
    case core::LogLevel::debug:
      ++summary.debugCount;
      break;
    case core::LogLevel::info:
      ++summary.infoCount;
      break;
    case core::LogLevel::warning:
      ++summary.warningCount;
      break;
    case core::LogLevel::error:
      ++summary.errorCount;
      break;
    case core::LogLevel::critical:
      ++summary.criticalCount;
      break;
    case core::LogLevel::unknown:
      ++summary.unknownCount;
      break;
    }
  }

  return summary;
}

} // namespace loginsight::parsers
