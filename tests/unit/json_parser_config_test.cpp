#include "loginsight/parsers/json_parser_config.hpp"

#include <array>
#include <optional>
#include <string_view>

#include <gtest/gtest.h>

using loginsight::parsers::JsonInputMode;
using loginsight::parsers::JsonParserConfig;
using loginsight::parsers::JsonParserConfigError;
using loginsight::parsers::validate_json_parser_config;

TEST(JsonParserConfigTest, DefaultsToConventionalJsonLinesFields) {
  const JsonParserConfig config;

  EXPECT_EQ(config.inputMode, JsonInputMode::json_lines);
  EXPECT_EQ(config.timestampFieldPath, "timestamp");
  EXPECT_EQ(config.levelFieldPath, "level");
  ASSERT_TRUE(config.sourceFieldPath.has_value());
  EXPECT_EQ(*config.sourceFieldPath, "source");
  EXPECT_EQ(config.messageFieldPath, "message");
  EXPECT_EQ(validate_json_parser_config(config), std::nullopt);
}

TEST(JsonParserConfigTest, AcceptsEveryDeclaredInputMode) {
  constexpr std::array modes{
      JsonInputMode::single_object,
      JsonInputMode::array_of_objects,
      JsonInputMode::json_lines,
  };

  for (const auto mode : modes) {
    JsonParserConfig config;
    config.inputMode = mode;

    EXPECT_EQ(validate_json_parser_config(config), std::nullopt);
  }
}

TEST(JsonParserConfigTest, AcceptsDotSeparatedNestedObjectPaths) {
  const JsonParserConfig config{
      .inputMode = JsonInputMode::array_of_objects,
      .timestampFieldPath = "event.created_at",
      .levelFieldPath = "event.severity",
      .sourceFieldPath = "service.name",
      .messageFieldPath = "event.description",
  };

  EXPECT_EQ(validate_json_parser_config(config), std::nullopt);
}

TEST(JsonParserConfigTest, RepresentsAnAbsentSourceWithNullopt) {
  JsonParserConfig config;
  config.sourceFieldPath = std::nullopt;

  EXPECT_FALSE(config.sourceFieldPath.has_value());
  EXPECT_EQ(validate_json_parser_config(config), std::nullopt);
}

TEST(JsonParserConfigTest, RejectsAnInvalidInputMode) {
  JsonParserConfig config;
  config.inputMode = static_cast<JsonInputMode>(999);

  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::invalid_input_mode);
}

TEST(JsonParserConfigTest, RejectsEmptyRequiredFieldPaths) {
  JsonParserConfig config;
  config.timestampFieldPath.clear();
  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::empty_timestamp_field_path);

  config = JsonParserConfig{};
  config.levelFieldPath.clear();
  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::empty_level_field_path);

  config = JsonParserConfig{};
  config.messageFieldPath.clear();
  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::empty_message_field_path);
}

TEST(JsonParserConfigTest, RejectsAnEngagedEmptySourceFieldPath) {
  JsonParserConfig config;
  config.sourceFieldPath = "";

  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::empty_source_field_path);
}

TEST(JsonParserConfigTest, RejectsEmptySegmentsInRequiredFieldPaths) {
  constexpr std::array malformed_paths{
      std::string_view{".event"},
      std::string_view{"event."},
      std::string_view{"event..created_at"},
  };

  for (const auto path : malformed_paths) {
    JsonParserConfig config;
    config.timestampFieldPath = path;

    EXPECT_EQ(validate_json_parser_config(config),
              JsonParserConfigError::malformed_timestamp_field_path);
  }

  JsonParserConfig config;
  config.levelFieldPath = "event..severity";
  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::malformed_level_field_path);

  config = JsonParserConfig{};
  config.messageFieldPath = "event.description.";
  EXPECT_EQ(validate_json_parser_config(config),
            JsonParserConfigError::malformed_message_field_path);
}

TEST(JsonParserConfigTest, RejectsAnEmptySegmentInTheSourceFieldPath) {
  JsonParserConfig config;
  config.sourceFieldPath = "service..name";

  EXPECT_EQ(validate_json_parser_config(config),
            JsonParserConfigError::malformed_source_field_path);
}

TEST(JsonParserConfigTest, RejectsDuplicateRequiredFieldPaths) {
  JsonParserConfig config;
  config.levelFieldPath = config.timestampFieldPath;

  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::duplicate_field_path);
}

TEST(JsonParserConfigTest, RejectsASourcePathThatDuplicatesARequiredPath) {
  JsonParserConfig config;
  config.sourceFieldPath = config.messageFieldPath;

  EXPECT_EQ(validate_json_parser_config(config), JsonParserConfigError::duplicate_field_path);
}
