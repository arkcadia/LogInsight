#include "loginsight/rules/error_burst.hpp"

#include <algorithm>
#include <map>

namespace loginsight::rules {

std::vector<ErrorBurst> find_error_bursts(std::span<const core::LogEntry* const> entries,
                                          const BurstRule& rule) {
  if (rule.threshold == 0 || rule.threshold > entries.size() || rule.window.count() <= 0 ||
      rule.window > std::chrono::hours{24})
    return {};
  std::map<std::string_view, std::vector<const core::LogEntry*>> bySource;
  for (const auto* entry : entries) {
    if (entry->level != core::LogLevel::unknown && entry->level >= rule.minimumLevel &&
        (rule.source.empty() || entry->source.name == rule.source))
      bySource[entry->source.name].push_back(entry);
  }

  std::vector<ErrorBurst> bursts;
  const auto earlier = [](const auto* a, const auto* b) { return a->timestamp < b->timestamp; };
  for (auto& [source, events] : bySource) {
    std::stable_sort(events.begin(), events.end(), earlier);
    auto first = events.cbegin();
    const auto threshold = static_cast<std::ptrdiff_t>(rule.threshold);
    const auto maximumStart = core::LogTimestamp::max() - rule.window;
    while (events.cend() - first >= threshold) {
      const auto start = (*first)->timestamp;
      const auto end = (*(first + threshold - 1))->timestamp;
      if (start > maximumStart || end <= start + rule.window) {
        bursts.emplace_back(first, first + threshold);
        first += threshold;
      } else {
        ++first;
      }
    }
  }
  std::stable_sort(bursts.begin(), bursts.end(),
                   [&](const auto& a, const auto& b) { return earlier(a.front(), b.front()); });
  return bursts;
}

} // namespace loginsight::rules
