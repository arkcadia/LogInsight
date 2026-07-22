#pragma once

#include "loginsight/parsers/parse_result.hpp"

#include <filesystem>

namespace loginsight::parsers {

class ParserRegistry;

enum class FileImportStatus {
  completed,
  unsupported_format,
};

// Parser selection is reported separately from errors collected during parsing.
struct FileImportResult {
  FileImportStatus status{FileImportStatus::unsupported_format};
  ParseResult parseResult{};

  [[nodiscard]] bool isSupported() const noexcept { return status == FileImportStatus::completed; }
};

class FileImportService {
public:
  explicit FileImportService(const ParserRegistry& registry) noexcept;

  [[nodiscard]] FileImportResult importFile(const std::filesystem::path& path) const;

private:
  const ParserRegistry& registry_;
};

} // namespace loginsight::parsers
