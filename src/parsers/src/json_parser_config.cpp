#include "loginsight/parsers/json_parser_config.hpp"

#include <string_view>

namespace loginsight::parsers {
namespace {

[[nodiscard]] bool is_valid_input_mode(JsonInputMode mode) noexcept {
  switch (mode) {
  case JsonInputMode::single_object:
  case JsonInputMode::array_of_objects:
  case JsonInputMode::json_lines:
    return true;
  }

  return false;
}

[[nodiscard]] bool is_well_formed_field_path(std::string_view path) noexcept {
  return !path.empty() && path.front() != '.' && path.back() != '.' &&
         path.find("..") == std::string_view::npos;
}

} // namespace

std::optional<JsonParserConfigError>
validate_json_parser_config(const JsonParserConfig& config) noexcept {
  if (!is_valid_input_mode(config.inputMode)) {
    return JsonParserConfigError::invalid_input_mode;
  }

  if (config.timestampFieldPath.empty()) {
    return JsonParserConfigError::empty_timestamp_field_path;
  }
  if (!is_well_formed_field_path(config.timestampFieldPath)) {
    return JsonParserConfigError::malformed_timestamp_field_path;
  }

  if (config.levelFieldPath.empty()) {
    return JsonParserConfigError::empty_level_field_path;
  }
  if (!is_well_formed_field_path(config.levelFieldPath)) {
    return JsonParserConfigError::malformed_level_field_path;
  }

  if (config.sourceFieldPath.has_value()) {
    if (config.sourceFieldPath->empty()) {
      return JsonParserConfigError::empty_source_field_path;
    }
    if (!is_well_formed_field_path(*config.sourceFieldPath)) {
      return JsonParserConfigError::malformed_source_field_path;
    }
  }

  if (config.messageFieldPath.empty()) {
    return JsonParserConfigError::empty_message_field_path;
  }
  if (!is_well_formed_field_path(config.messageFieldPath)) {
    return JsonParserConfigError::malformed_message_field_path;
  }

  const std::string_view timestamp_path{config.timestampFieldPath};
  const std::string_view level_path{config.levelFieldPath};
  const std::string_view message_path{config.messageFieldPath};
  if (timestamp_path == level_path || timestamp_path == message_path ||
      level_path == message_path) {
    return JsonParserConfigError::duplicate_field_path;
  }

  if (config.sourceFieldPath.has_value()) {
    const std::string_view source_path{*config.sourceFieldPath};
    if (source_path == timestamp_path || source_path == level_path || source_path == message_path) {
      return JsonParserConfigError::duplicate_field_path;
    }
  }

  return std::nullopt;
}

} // namespace loginsight::parsers
