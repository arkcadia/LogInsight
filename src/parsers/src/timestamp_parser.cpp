#include "loginsight/parsers/timestamp_parser.hpp"

#include <cstddef>

namespace loginsight::parsers {
namespace {

constexpr std::size_t date_size = 10;
constexpr std::size_t time_size = 8;
constexpr std::size_t timestamp_size = date_size + 1 + time_size;
constexpr std::size_t timestamp_with_utc_size = timestamp_size + 1;

[[nodiscard]] bool is_digit(char value) noexcept { return value >= '0' && value <= '9'; }

[[nodiscard]] std::optional<int> parse_fixed_int(std::string_view value, std::size_t offset,
                                                 std::size_t count) noexcept {
  if (offset + count > value.size()) {
    return std::nullopt;
  }

  int result = 0;
  for (std::size_t index = offset; index < offset + count; ++index) {
    if (!is_digit(value[index])) {
      return std::nullopt;
    }

    result = (result * 10) + static_cast<int>(value[index] - '0');
  }

  return result;
}

[[nodiscard]] bool has_supported_shape(std::string_view value) noexcept {
  if (value.size() != timestamp_size && value.size() != timestamp_with_utc_size) {
    return false;
  }

  const auto separator = value[date_size];
  if (separator != ' ' && separator != 'T') {
    return false;
  }

  if (value.size() == timestamp_with_utc_size && value.back() != 'Z') {
    return false;
  }

  return value[4] == '-' && value[7] == '-' && value[13] == ':' && value[16] == ':';
}

} // namespace

std::optional<std::chrono::system_clock::time_point>
parse_timestamp(std::string_view value) noexcept {
  if (!has_supported_shape(value)) {
    return std::nullopt;
  }

  const auto year_value = parse_fixed_int(value, 0, 4);
  const auto month_value = parse_fixed_int(value, 5, 2);
  const auto day_value = parse_fixed_int(value, 8, 2);
  const auto hour_value = parse_fixed_int(value, 11, 2);
  const auto minute_value = parse_fixed_int(value, 14, 2);
  const auto second_value = parse_fixed_int(value, 17, 2);

  if (!year_value.has_value() || !month_value.has_value() || !day_value.has_value() ||
      !hour_value.has_value() || !minute_value.has_value() || !second_value.has_value()) {
    return std::nullopt;
  }

  if (*hour_value > 23 || *minute_value > 59 || *second_value > 59) {
    return std::nullopt;
  }

  const auto date = std::chrono::year{*year_value} /
                    std::chrono::month{static_cast<unsigned>(*month_value)} /
                    std::chrono::day{static_cast<unsigned>(*day_value)};
  if (!date.ok()) {
    return std::nullopt;
  }

  const auto timestamp = std::chrono::sys_days{date} + std::chrono::hours{*hour_value} +
                         std::chrono::minutes{*minute_value} + std::chrono::seconds{*second_value};
  return std::chrono::time_point_cast<std::chrono::system_clock::duration>(timestamp);
}

} // namespace loginsight::parsers
