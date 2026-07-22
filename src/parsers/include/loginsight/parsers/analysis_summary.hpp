#pragma once

#include "loginsight/parsers/parse_result.hpp"

#include <cstddef>

namespace loginsight::parsers {

struct AnalysisSummary {
  std::size_t validEntryCount{};
  std::size_t malformedLineCount{};
  std::size_t traceCount{};
  std::size_t debugCount{};
  std::size_t infoCount{};
  std::size_t warningCount{};
  std::size_t errorCount{};
  std::size_t criticalCount{};
  std::size_t unknownCount{};
};

[[nodiscard]] AnalysisSummary build_analysis_summary(const ParseResult& result) noexcept;

} // namespace loginsight::parsers
