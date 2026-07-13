#pragma once

#include <chrono>
#include <optional>
#include <string_view>

namespace loginsight::parsers {

// Supported timestamp strings are normalized to system_clock values. Formats without an explicit
// timezone suffix are interpreted with the same UTC convention as timestamps ending in 'Z'.
[[nodiscard]] std::optional<std::chrono::system_clock::time_point>
parse_timestamp(std::string_view value) noexcept;

} // namespace loginsight::parsers
