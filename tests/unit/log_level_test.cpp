#include "loginsight/core/log_level.hpp"

#include <gtest/gtest.h>

using loginsight::core::log_level_from_string;
using loginsight::core::LogLevel;
using loginsight::core::to_string;

TEST(LogLevelTest, ConvertsLevelsToCanonicalStrings) {
  EXPECT_EQ(to_string(LogLevel::trace), "trace");
  EXPECT_EQ(to_string(LogLevel::debug), "debug");
  EXPECT_EQ(to_string(LogLevel::info), "info");
  EXPECT_EQ(to_string(LogLevel::warning), "warning");
  EXPECT_EQ(to_string(LogLevel::error), "error");
  EXPECT_EQ(to_string(LogLevel::critical), "critical");
  EXPECT_EQ(to_string(LogLevel::unknown), "unknown");
}

TEST(LogLevelTest, ParsesCanonicalStringsCaseInsensitively) {
  EXPECT_EQ(log_level_from_string("TRACE"), LogLevel::trace);
  EXPECT_EQ(log_level_from_string("debug"), LogLevel::debug);
  EXPECT_EQ(log_level_from_string("Info"), LogLevel::info);
  EXPECT_EQ(log_level_from_string("WARNING"), LogLevel::warning);
  EXPECT_EQ(log_level_from_string("Error"), LogLevel::error);
  EXPECT_EQ(log_level_from_string("critical"), LogLevel::critical);
  EXPECT_EQ(log_level_from_string("unknown"), LogLevel::unknown);
}

TEST(LogLevelTest, ParsesCommonAliases) {
  EXPECT_EQ(log_level_from_string("information"), LogLevel::info);
  EXPECT_EQ(log_level_from_string("warn"), LogLevel::warning);
  EXPECT_EQ(log_level_from_string("err"), LogLevel::error);
  EXPECT_EQ(log_level_from_string("fatal"), LogLevel::critical);
}

TEST(LogLevelTest, RejectsUnrecognizedStrings) {
  EXPECT_EQ(log_level_from_string(""), std::nullopt);
  EXPECT_EQ(log_level_from_string("notice"), std::nullopt);
}
