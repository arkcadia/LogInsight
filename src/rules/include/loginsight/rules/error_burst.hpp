#pragma once

#include "loginsight/core/log_entry.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace loginsight::rules {

using ErrorBurst = std::vector<const core::LogEntry*>;

struct BurstRule {
  std::size_t threshold{5};
  std::chrono::seconds window{60};
  core::LogLevel minimumLevel{core::LogLevel::error};
  std::string source;
};

// Finds non-overlapping groups from a single source satisfying the rule.
[[nodiscard]] std::vector<ErrorBurst>
find_error_bursts(std::span<const core::LogEntry* const> entries, const BurstRule& rule = {});

} // namespace loginsight::rules
