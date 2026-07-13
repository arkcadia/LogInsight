#include "loginsight/core/log_entry.hpp"

#include <chrono>

#include <gtest/gtest.h>

using loginsight::core::LogEntry;
using loginsight::core::LogLevel;
using loginsight::core::LogSource;

TEST(LogEntryTest, DefaultsToUnknownLevelAndEmptyFields) {
  const LogEntry entry;

  EXPECT_EQ(entry.level, LogLevel::unknown);
  EXPECT_TRUE(entry.message.empty());
  EXPECT_TRUE(entry.source.name.empty());
  EXPECT_TRUE(entry.attributes.empty());
}

TEST(LogEntryTest, StoresNormalizedLogData) {
  const auto timestamp = std::chrono::system_clock::time_point{std::chrono::seconds{42}};
  const LogSource source{
      .name = "auth.log",
      .host = "workstation",
      .application = "sshd",
      .file_path = "/var/log/auth.log",
  };
  const LogEntry entry{
      .timestamp = timestamp,
      .level = LogLevel::warning,
      .source = source,
      .message = "Failed password",
      .attributes = {{"pid", "1234"}, {"user", "alice"}},
  };

  EXPECT_EQ(entry.timestamp, timestamp);
  EXPECT_EQ(entry.level, LogLevel::warning);
  EXPECT_EQ(entry.source, source);
  EXPECT_EQ(entry.message, "Failed password");
  EXPECT_EQ(entry.attributes.at("pid"), "1234");
  EXPECT_EQ(entry.attributes.at("user"), "alice");
}
