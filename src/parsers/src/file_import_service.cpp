#include "loginsight/parsers/file_import_service.hpp"

#include "loginsight/parsers/parser_registry.hpp"

namespace loginsight::parsers {

FileImportService::FileImportService(const ParserRegistry& registry) noexcept
    : registry_{registry} {}

FileImportResult FileImportService::importFile(const std::filesystem::path& path) const {
  const auto* parser = registry_.findParser(path);
  if (parser == nullptr) {
    return {};
  }

  return FileImportResult{
      .status = FileImportStatus::completed,
      .parseResult = parser->parse(path),
  };
}

} // namespace loginsight::parsers
