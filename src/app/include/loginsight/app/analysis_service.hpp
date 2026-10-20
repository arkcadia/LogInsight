#pragma once

#include "loginsight/parsers/analysis_summary.hpp"
#include "loginsight/parsers/file_import_service.hpp"
#include "loginsight/parsers/parser_registry.hpp"

#include <filesystem>
#include <iosfwd>
#include <span>
#include <string>

namespace loginsight::app {

enum class AnalysisStatus { completed, unsupported_format, file_not_found, io_failure };

struct AnalysisResult {
  std::filesystem::path path;
  AnalysisStatus status{AnalysisStatus::io_failure};
  parsers::ParseResult parsed;
  parsers::AnalysisSummary summary;
  std::string diagnostic;
  [[nodiscard]] bool completed() const noexcept { return status == AnalysisStatus::completed; }
};

// Shared by the CLI and desktop; no presentation dependencies.
[[nodiscard]] AnalysisResult analyze_file(const std::filesystem::path& path,
                                          const parsers::FileImportService& importer);
class AnalysisService {
public:
  AnalysisService();
  [[nodiscard]] AnalysisResult analyze(const std::filesystem::path& path) const;

private:
  parsers::ParserRegistry registry_;
};

[[nodiscard]] std::string format_timestamp(core::LogTimestamp timestamp);
// Streams the whole session (including malformed records); throws on output failure.
void write_json_report(std::ostream& output, std::span<const AnalysisResult> results);
// Does not overwrite an existing destination. Stages a temporary sibling, then publishes with an
// exclusive hard link.
void save_json_report(const std::filesystem::path& destination,
                      std::span<const AnalysisResult> results);

} // namespace loginsight::app
