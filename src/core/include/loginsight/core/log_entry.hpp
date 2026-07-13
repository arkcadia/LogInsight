#pragma once

#include "loginsight/core/log_level.hpp"
#include "loginsight/core/log_source.hpp"

#include <chrono>
#include <map>
#include <string>

namespace loginsight::core {

using LogAttributes = std::map<std::string, std::string>;
using LogTimestamp = std::chrono::system_clock::time_point;

struct LogEntry {
  LogTimestamp timestamp{};
  LogLevel level{LogLevel::unknown};
  LogSource source{};
  std::string message{};
  LogAttributes attributes{};

  friend bool operator==(const LogEntry&, const LogEntry&) = default;
};

} // namespace loginsight::core
