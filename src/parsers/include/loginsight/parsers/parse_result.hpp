#pragma once

#include "loginsight/core/log_entry.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace loginsight::parsers {

struct ParseError {
  std::size_t lineNumber{};
  std::string rawContent{};
  std::string message{};

  friend bool operator==(const ParseError&, const ParseError&) = default;
};

struct ParseResult {
  std::vector<core::LogEntry> entries{};
  std::vector<ParseError> errors{};

  [[nodiscard]] bool has_errors() const noexcept { return !errors.empty(); }
};

} // namespace loginsight::parsers
