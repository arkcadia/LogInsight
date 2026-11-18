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
parse_timestamp(std::string_view value, bool allowSubsecondsAndOffset) noexcept {
  using namespace std::chrono;
  int fractionalNanoseconds = 0;
  minutes offset{};
  if (allowSubsecondsAndOffset && value.size() > timestamp_size) {
    auto suffix = value.substr(timestamp_size);
    value = value.substr(0, timestamp_size);
    if (suffix.starts_with('.')) {
      suffix.remove_prefix(1);
      std::size_t digits = 0;
      while (digits < suffix.size() && is_digit(suffix[digits]))
        ++digits;
      if (digits == 0 || digits > 9)
        return std::nullopt;
      fractionalNanoseconds = *parse_fixed_int(suffix, 0, digits);
      suffix.remove_prefix(digits);
      for (; digits < 9; ++digits)
        fractionalNanoseconds *= 10;
    }
    if (!suffix.empty() && suffix != "Z") {
      if (suffix.size() != 6 || (suffix.front() != '+' && suffix.front() != '-') ||
          suffix[3] != ':')
        return std::nullopt;
      const auto hoursValue = parse_fixed_int(suffix, 1, 2);
      const auto minutesValue = parse_fixed_int(suffix, 4, 2);
      if (!hoursValue || *hoursValue > 23 || !minutesValue || *minutesValue > 59)
        return std::nullopt;
      offset = hours{*hoursValue} + minutes{*minutesValue};
      if (suffix.front() == '-')
        offset = -offset;
    }
  }
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
                         std::chrono::minutes{*minute_value} + std::chrono::seconds{*second_value} -
                         offset;
  // Compare the whole-second part before converting it to a finer clock duration.
  using Clock = system_clock;
  const auto fraction = duration_cast<Clock::duration>(nanoseconds{fractionalNanoseconds});
  const auto minimum = Clock::time_point::min();
  const auto maximum = Clock::time_point::max();
  const auto firstWhole = ceil<seconds>(minimum);
  const auto lastWhole = floor<seconds>(maximum);
  if (timestamp < floor<seconds>(minimum) || timestamp > lastWhole)
    return std::nullopt;
  if (timestamp < firstWhole) {
    const auto next = time_point_cast<Clock::duration>(firstWhole);
    const auto back = seconds{1} - fraction;
    if (back > next - minimum)
      return std::nullopt;
    return next - back;
  }
  const auto whole = time_point_cast<Clock::duration>(timestamp);
  if (timestamp == lastWhole && fraction > maximum - whole)
    return std::nullopt;
  return whole + fraction;
}

} // namespace loginsight::parsers
