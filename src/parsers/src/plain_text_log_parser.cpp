#include "loginsight/parsers/plain_text_log_parser.hpp"

#include "loginsight/core/log_level.hpp"

#include <cctype>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace loginsight::parsers {
namespace {

constexpr std::size_t date_size = 10;
constexpr std::size_t time_size = 8;
constexpr std::size_t timestamp_size = date_size + 1 + time_size;
constexpr std::size_t level_offset = timestamp_size + 1;

struct LineParseSuccess {
  core::LogEntry entry;
};

struct LineParseFailure {
  std::string message;
};

using LineParseResult = std::variant<LineParseSuccess, LineParseFailure>;

[[nodiscard]] bool ascii_equal_ignore_case(std::string_view lhs, std::string_view rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (std::size_t index = 0; index < lhs.size(); ++index) {
    const auto left = static_cast<unsigned char>(lhs[index]);
    const auto right = static_cast<unsigned char>(rhs[index]);
    if (std::tolower(left) != std::tolower(right)) {
      return false;
    }
  }

  return true;
}

[[nodiscard]] bool is_digit(char value) noexcept { return value >= '0' && value <= '9'; }

[[nodiscard]] std::optional<int> parse_fixed_int(std::string_view value, std::size_t offset,
                                                 std::size_t count) {
  if (offset + count > value.size()) {
    return std::nullopt;
  }

  int result = 0;
  for (std::size_t index = offset; index < offset + count; ++index) {
    if (!is_digit(value[index])) {
      return std::nullopt;
    }

    result = (result * 10) + static_cast<int>(value[index] - '0');
  }

  return result;
}

[[nodiscard]] std::optional<core::LogTimestamp> parse_timestamp(std::string_view value) {
  if (value.size() != timestamp_size || value[4] != '-' || value[7] != '-' ||
      value[date_size] != ' ' || value[13] != ':' || value[16] != ':') {
    return std::nullopt;
  }

  const auto year_value = parse_fixed_int(value, 0, 4);
  const auto month_value = parse_fixed_int(value, 5, 2);
  const auto day_value = parse_fixed_int(value, 8, 2);
  const auto hour_value = parse_fixed_int(value, 11, 2);
  const auto minute_value = parse_fixed_int(value, 14, 2);
  const auto second_value = parse_fixed_int(value, 17, 2);

  if (!year_value.has_value() || !month_value.has_value() || !day_value.has_value() ||
      !hour_value.has_value() || !minute_value.has_value() || !second_value.has_value()) {
    return std::nullopt;
  }

  if (*hour_value > 23 || *minute_value > 59 || *second_value > 59) {
    return std::nullopt;
  }

  const auto date = std::chrono::year{*year_value} /
                    std::chrono::month{static_cast<unsigned>(*month_value)} /
                    std::chrono::day{static_cast<unsigned>(*day_value)};
  if (!date.ok()) {
    return std::nullopt;
  }

  const auto timestamp = std::chrono::sys_days{date} + std::chrono::hours{*hour_value} +
                         std::chrono::minutes{*minute_value} + std::chrono::seconds{*second_value};
  return std::chrono::time_point_cast<core::LogTimestamp::duration>(timestamp);
}

[[nodiscard]] std::optional<core::LogLevel> parse_plain_text_level(std::string_view value) {
  if (ascii_equal_ignore_case(value, "TRACE")) {
    return core::LogLevel::trace;
  }
  if (ascii_equal_ignore_case(value, "DEBUG")) {
    return core::LogLevel::debug;
  }
  if (ascii_equal_ignore_case(value, "INFO")) {
    return core::LogLevel::info;
  }
  if (ascii_equal_ignore_case(value, "WARNING") || ascii_equal_ignore_case(value, "WARN")) {
    return core::LogLevel::warning;
  }
  if (ascii_equal_ignore_case(value, "ERROR")) {
    return core::LogLevel::error;
  }
  if (ascii_equal_ignore_case(value, "CRITICAL")) {
    return core::LogLevel::critical;
  }

  return std::nullopt;
}

[[nodiscard]] bool has_non_space(std::string_view value) {
  for (const auto character : value) {
    if (character != ' ') {
      return true;
    }
  }

  return false;
}

[[nodiscard]] LineParseResult parse_line(std::string_view line, std::string_view source_name) {
  if (line.size() <= level_offset) {
    return LineParseFailure{"line is too short for the expected plain text format"};
  }

  const auto timestamp = parse_timestamp(line.substr(0, timestamp_size));
  if (!timestamp.has_value()) {
    return LineParseFailure{"invalid timestamp"};
  }

  if (line[timestamp_size] != ' ') {
    return LineParseFailure{"missing separator after timestamp"};
  }

  const auto level_end = line.find(' ', level_offset);
  if (level_end == std::string_view::npos) {
    return LineParseFailure{"missing message"};
  }

  const auto level = parse_plain_text_level(line.substr(level_offset, level_end - level_offset));
  if (!level.has_value()) {
    return LineParseFailure{"unsupported log level"};
  }

  const auto message = line.substr(level_end + 1);
  if (message.empty() || !has_non_space(message)) {
    return LineParseFailure{"missing message"};
  }

  return LineParseSuccess{core::LogEntry{
      .timestamp = *timestamp,
      .level = *level,
      .source = core::LogSource{.name = std::string{source_name}},
      .message = std::string{message},
      .rawContent = std::string{line},
      .attributes = {},
  }};
}

} // namespace

bool PlainTextLogParser::canParse(const std::filesystem::path& path) const {
  const auto extension = path.extension().string();
  return ascii_equal_ignore_case(extension, ".log") || ascii_equal_ignore_case(extension, ".txt");
}

ParseResult PlainTextLogParser::parse(const std::filesystem::path& path) const {
  std::ifstream file{path};
  if (!file.is_open()) {
    return ParseResult{
        .entries = {},
        .errors = {ParseError{
            .lineNumber = 0U,
            .rawContent = path.string(),
            .message = "unable to open file",
        }},
    };
  }

  return parse(file, path.filename().string());
}

ParseResult PlainTextLogParser::parse(std::istream& input, std::string_view source_name) const {
  ParseResult result;
  std::string line;
  std::size_t line_number = 0;

  while (std::getline(input, line)) {
    ++line_number;

    const auto parsed_line = parse_line(line, source_name);
    if (const auto* success = std::get_if<LineParseSuccess>(&parsed_line)) {
      result.entries.push_back(success->entry);
      continue;
    }

    const auto* failure = std::get_if<LineParseFailure>(&parsed_line);
    result.errors.push_back(ParseError{
        .lineNumber = line_number,
        .rawContent = line,
        .message = failure->message,
    });
  }

  return result;
}

} // namespace loginsight::parsers
