#pragma once

#include <optional>
#include <string_view>

namespace loginsight::core {

enum class LogLevel {
  trace,
  debug,
  info,
  warning,
  error,
  critical,
  unknown,
};

[[nodiscard]] std::string_view to_string(LogLevel level) noexcept;
[[nodiscard]] std::optional<LogLevel> log_level_from_string(std::string_view value);

} // namespace loginsight::core
