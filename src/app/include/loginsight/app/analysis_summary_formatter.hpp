#pragma once

#include "loginsight/parsers/analysis_summary.hpp"

#include <filesystem>
#include <string>

namespace loginsight::app {

[[nodiscard]] std::string format_analysis_summary(const std::filesystem::path& path,
                                                  const parsers::AnalysisSummary& summary);

} // namespace loginsight::app
