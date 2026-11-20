#pragma once

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <stop_token>
#include <string>

namespace loginsight::core {
struct LogEntry;
}

namespace loginsight::parsers {

struct ParseError;
struct ParseResult;

// Shared by all files in a bounded session, including blank and malformed records.
struct ImportBudget {
  static constexpr std::size_t maxBytes = 32U * 1024U * 1024U;
  static constexpr std::size_t maxLines = 200'000U;
  static constexpr std::size_t maxLineBytes = 256U * 1024U;

  std::stop_token stop;
  std::function<void(std::size_t)> progress;
  std::size_t bytes{};
  std::size_t lines{};
  bool enforceBatchLimits{true};
  std::function<void(const core::LogEntry&)> onEntry{};
  std::function<void(const ParseError&)> onMalformed{};

  void checkCancelled() const;
  bool readLine(std::istream& input, std::string& line);
};

void collect_entry(ParseResult& result, ImportBudget* budget, core::LogEntry entry);
void collect_malformed(ParseResult& result, ImportBudget* budget, ParseError error);

} // namespace loginsight::parsers
