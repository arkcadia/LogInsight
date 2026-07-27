#pragma once

#include <optional>
#include <string>

namespace loginsight::parsers {

// Describes the top-level shape of the JSON input. In every mode, each log record must be a JSON
// object; only the container representation differs.
enum class JsonInputMode {
  single_object,
  array_of_objects,
  json_lines,
};

enum class JsonParserConfigError {
  invalid_input_mode,
  empty_timestamp_field_path,
  malformed_timestamp_field_path,
  empty_level_field_path,
  malformed_level_field_path,
  empty_source_field_path,
  malformed_source_field_path,
  empty_message_field_path,
  malformed_message_field_path,
  duplicate_field_path,
};

// Defaults describe conventional JSON log records with top-level timestamp, level, source, and
// message members. JSON Lines is the default because it is the usual stream-friendly log format.
//
// Field paths use dots to traverse nested object members, for example "event.created_at". Empty
// path segments are invalid. The first implementation reserves dots as separators and does not
// support escaping literal dots, array indexing, or any other path expression syntax.
//
// A missing sourceFieldPath means that records do not provide a source field. An engaged empty
// string is invalid so absence has one unambiguous representation.
struct JsonParserConfig {
  JsonInputMode inputMode{JsonInputMode::json_lines};
  std::string timestampFieldPath{"timestamp"};
  std::string levelFieldPath{"level"};
  std::optional<std::string> sourceFieldPath{std::string{"source"}};
  std::string messageFieldPath{"message"};

  friend bool operator==(const JsonParserConfig&, const JsonParserConfig&) = default;
};

// Returns the first validation error in mode and field order, or std::nullopt for a valid
// configuration. Required paths must be non-empty, every path segment must be non-empty, and all
// configured paths must be unique. Paths are compared exactly and case-sensitively.
[[nodiscard]] std::optional<JsonParserConfigError>
validate_json_parser_config(const JsonParserConfig& config) noexcept;

} // namespace loginsight::parsers
