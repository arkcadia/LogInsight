#pragma once

#include "loginsight/parsers/import_budget.hpp"
#include "loginsight/parsers/parse_result.hpp"

#include <stdexcept>

#include <filesystem>

namespace loginsight::parsers {

class ILogParser {
public:
  virtual ~ILogParser() = default;

  [[nodiscard]] virtual bool canParse(const std::filesystem::path& path) const = 0;
  [[nodiscard]] virtual ParseResult parse(const std::filesystem::path& path) const = 0;
  [[nodiscard]] virtual ParseResult parse(const std::filesystem::path& path,
                                          ImportBudget* budget) const {
    if (budget)
      throw std::runtime_error("parser does not support bounded imports");
    return parse(path);
  }
};

} // namespace loginsight::parsers
