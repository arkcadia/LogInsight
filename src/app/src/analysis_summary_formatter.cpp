#include "loginsight/app/analysis_summary_formatter.hpp"

#include <string>

namespace loginsight::app {

std::string format_analysis_summary(const std::filesystem::path& path,
                                    const parsers::AnalysisSummary& summary) {
  std::string output{"Analysis complete\n\nFile: "};
  output += path.string();
  output += "\nValid entries: ";
  output += std::to_string(summary.validEntryCount);
  output += "\nMalformed lines: ";
  output += std::to_string(summary.malformedLineCount);
  output += "\n\nTrace: ";
  output += std::to_string(summary.traceCount);
  output += "\nDebug: ";
  output += std::to_string(summary.debugCount);
  output += "\nInfo: ";
  output += std::to_string(summary.infoCount);
  output += "\nWarning: ";
  output += std::to_string(summary.warningCount);
  output += "\nError: ";
  output += std::to_string(summary.errorCount);
  output += "\nCritical: ";
  output += std::to_string(summary.criticalCount);
  output += "\nUnknown: ";
  output += std::to_string(summary.unknownCount);
  output += '\n';
  return output;
}

} // namespace loginsight::app
