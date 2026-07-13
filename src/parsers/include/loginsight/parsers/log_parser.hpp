#pragma once

#include "loginsight/parsers/parse_result.hpp"

#include <filesystem>

namespace loginsight::parsers {

class ILogParser {
public:
  virtual ~ILogParser() = default;

  [[nodiscard]] virtual bool canParse(const std::filesystem::path& path) const = 0;
  [[nodiscard]] virtual ParseResult parse(const std::filesystem::path& path) const = 0;
};

} // namespace loginsight::parsers
