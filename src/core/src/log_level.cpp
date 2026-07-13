#include "loginsight/core/log_level.hpp"

#include <array>
#include <cctype>
#include <string_view>

namespace loginsight::core {
namespace {

struct LogLevelMapping {
  std::string_view text;
  LogLevel level;
};

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

constexpr std::array mappings{
    LogLevelMapping{"trace", LogLevel::trace},       LogLevelMapping{"debug", LogLevel::debug},
    LogLevelMapping{"info", LogLevel::info},         LogLevelMapping{"information", LogLevel::info},
    LogLevelMapping{"warn", LogLevel::warning},      LogLevelMapping{"warning", LogLevel::warning},
    LogLevelMapping{"err", LogLevel::error},         LogLevelMapping{"error", LogLevel::error},
    LogLevelMapping{"critical", LogLevel::critical}, LogLevelMapping{"fatal", LogLevel::critical},
    LogLevelMapping{"unknown", LogLevel::unknown},
};

} // namespace

std::string_view to_string(LogLevel level) noexcept {
  switch (level) {
  case LogLevel::trace:
    return "trace";
  case LogLevel::debug:
    return "debug";
  case LogLevel::info:
    return "info";
  case LogLevel::warning:
    return "warning";
  case LogLevel::error:
    return "error";
  case LogLevel::critical:
    return "critical";
  case LogLevel::unknown:
    return "unknown";
  }

  return "unknown";
}

std::optional<LogLevel> log_level_from_string(std::string_view value) {
  for (const auto& mapping : mappings) {
    if (ascii_equal_ignore_case(value, mapping.text)) {
      return mapping.level;
    }
  }

  return std::nullopt;
}

} // namespace loginsight::core
