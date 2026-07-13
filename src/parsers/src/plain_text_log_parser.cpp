#include "loginsight/parsers/plain_text_log_parser.hpp"

#include "loginsight/core/log_level.hpp"
#include "loginsight/parsers/timestamp_parser.hpp"

#include <cctype>
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

[[nodiscard]] bool has_non_space(std::string_view value) noexcept {
  for (const auto character : value) {
    if (character != ' ') {
      return true;
    }
  }

  return false;
}

[[nodiscard]] std::size_t skip_spaces(std::string_view value, std::size_t offset) noexcept {
  while (offset < value.size() && value[offset] == ' ') {
    ++offset;
  }

  return offset;
}

struct Token {
  std::string_view value;
  std::size_t next_offset{};
};

[[nodiscard]] std::optional<Token> read_token(std::string_view line, std::size_t offset) noexcept {
  offset = skip_spaces(line, offset);
  if (offset >= line.size()) {
    return std::nullopt;
  }

  const auto token_end = line.find(' ', offset);
  if (token_end == std::string_view::npos) {
    return Token{.value = line.substr(offset), .next_offset = line.size()};
  }

  return Token{.value = line.substr(offset, token_end - offset), .next_offset = token_end};
}

[[nodiscard]] LineParseResult parse_line(std::string_view line) {
  if (line.size() < timestamp_size) {
    return LineParseFailure{"invalid timestamp"};
  }

  const auto timestamp = parse_timestamp(line.substr(0, timestamp_size));
  if (!timestamp.has_value()) {
    return LineParseFailure{"invalid timestamp"};
  }

  auto offset = timestamp_size;
  if (offset < line.size() && line[offset] != ' ') {
    return LineParseFailure{"missing separator after timestamp"};
  }

  const auto level_token = read_token(line, offset);
  if (!level_token.has_value()) {
    return LineParseFailure{"missing level"};
  }

  const auto level = parse_plain_text_level(level_token->value);
  if (!level.has_value()) {
    return LineParseFailure{"unsupported log level"};
  }

  const auto source_token = read_token(line, level_token->next_offset);
  if (!source_token.has_value()) {
    return LineParseFailure{"missing source"};
  }

  offset = skip_spaces(line, source_token->next_offset);
  if (offset >= line.size()) {
    return LineParseFailure{"missing message"};
  }

  const auto message = line.substr(offset);
  if (message.empty() || !has_non_space(message)) {
    return LineParseFailure{"missing message"};
  }

  return LineParseSuccess{core::LogEntry{
      .timestamp = *timestamp,
      .level = *level,
      .source = core::LogSource{.name = std::string{source_token->value}},
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

  return parse(file);
}

ParseResult PlainTextLogParser::parse(std::istream& input) const {
  ParseResult result;
  std::string line;
  std::size_t line_number = 0;

  while (std::getline(input, line)) {
    ++line_number;

    if (!has_non_space(line)) {
      continue;
    }

    const auto parsed_line = parse_line(line);
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
