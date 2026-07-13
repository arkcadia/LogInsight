#pragma once

#include "loginsight/core/log_entry.hpp"

#include <cstddef>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

namespace loginsight::parsers {

struct ParseError {
  std::size_t line_number{};
  std::string line{};
  std::string message{};

  friend bool operator==(const ParseError&, const ParseError&) = default;
};

struct ParseResult {
  std::vector<core::LogEntry> entries{};
  std::vector<ParseError> errors{};

  [[nodiscard]] bool has_errors() const noexcept { return !errors.empty(); }
};

class ILogParser {
public:
  virtual ~ILogParser() = default;

  [[nodiscard]] virtual ParseResult parse(std::istream& input,
                                          std::string_view source_name = {}) const = 0;
};

} // namespace loginsight::parsers
