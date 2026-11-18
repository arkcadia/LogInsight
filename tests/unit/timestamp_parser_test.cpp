#include "loginsight/parsers/timestamp_parser.hpp"

#include <chrono>
#include <optional>

#include <gtest/gtest.h>

using loginsight::parsers::parse_timestamp;

namespace {

[[nodiscard]] std::chrono::system_clock::time_point
timestamp(int year, unsigned month, unsigned day, int hour, int minute, int second) {
  const auto date = std::chrono::year{year} / std::chrono::month{month} / std::chrono::day{day};
  const auto value = std::chrono::sys_days{date} + std::chrono::hours{hour} +
                     std::chrono::minutes{minute} + std::chrono::seconds{second};
  return std::chrono::time_point_cast<std::chrono::system_clock::duration>(value);
}

} // namespace

TEST(TimestampParserTest, ParsesValidTimestampWithSpaceSeparator) {
  const auto parsed = parse_timestamp("2026-07-12 15:21:04");

  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(*parsed, timestamp(2026, 7U, 12U, 15, 21, 4));
}

TEST(TimestampParserTest, ParsesValidIsoStyleTimestamp) {
  const auto parsed = parse_timestamp("2026-07-12T15:21:04");

  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(*parsed, timestamp(2026, 7U, 12U, 15, 21, 4));
}

TEST(TimestampParserTest, ParsesValidUtcTimestampEndingInZ) {
  const auto parsed = parse_timestamp("2026-07-12T15:21:04Z");

  ASSERT_TRUE(parsed.has_value());
  EXPECT_EQ(*parsed, timestamp(2026, 7U, 12U, 15, 21, 4));
}

TEST(TimestampParserTest, RejectsInvalidMonth) {
  EXPECT_EQ(parse_timestamp("2026-13-12T15:21:04"), std::nullopt);
  EXPECT_EQ(parse_timestamp("2026-00-12T15:21:04"), std::nullopt);
}

TEST(TimestampParserTest, RejectsInvalidDay) {
  EXPECT_EQ(parse_timestamp("2026-02-30T15:21:04"), std::nullopt);
  EXPECT_EQ(parse_timestamp("2026-07-00T15:21:04"), std::nullopt);
}

TEST(TimestampParserTest, RejectsInvalidHour) {
  EXPECT_EQ(parse_timestamp("2026-07-12T24:21:04"), std::nullopt);
}

TEST(TimestampParserTest, RejectsMissingFields) {
  EXPECT_EQ(parse_timestamp("2026-07-12"), std::nullopt);
  EXPECT_EQ(parse_timestamp("2026-07-12T15:21"), std::nullopt);
  EXPECT_EQ(parse_timestamp("2026-07-12T15:21:"), std::nullopt);
}

TEST(TimestampParserTest, RejectsEmptyString) { EXPECT_EQ(parse_timestamp(""), std::nullopt); }

TEST(TimestampParserTest, RejectsTrailingUnexpectedCharacters) {
  EXPECT_EQ(parse_timestamp("2026-07-12T15:21:04+01:00"), std::nullopt);
  EXPECT_EQ(parse_timestamp("2026-07-12T15:21:04ZZ"), std::nullopt);
  EXPECT_EQ(parse_timestamp("2026-07-12 15:21:04 extra"), std::nullopt);
}

TEST(TimestampParserTest, RejectsUnsupportedSeparatorsAndTimezoneMarkers) {
  EXPECT_EQ(parse_timestamp("2026-07-12_15:21:04"), std::nullopt);
  EXPECT_EQ(parse_timestamp("2026-07-12T15:21:04z"), std::nullopt);
}

TEST(TimestampParserTest, RejectsDatesOutsideSystemClockRangeWithoutOverflow) {
  // Linux system_clock stores nanoseconds in a signed 64-bit value.
  if constexpr (std::chrono::system_clock::period::den >= 1000000000) {
    EXPECT_EQ(parse_timestamp("9999-12-31T23:59:59"), std::nullopt);
    EXPECT_EQ(parse_timestamp("0001-01-01T00:00:00"), std::nullopt);
  }
  EXPECT_TRUE(parse_timestamp("2000-02-29T00:00:00").has_value());
  EXPECT_EQ(parse_timestamp("1900-02-29T00:00:00"), std::nullopt);
}

TEST(TimestampParserTest, ExtendedFormatNormalizesOffsetsAndValidatesFractions) {
  using namespace std::chrono;
  EXPECT_EQ(parse_timestamp("1970-01-01T00:59:59.25+01:00", true),
            system_clock::time_point{milliseconds{-750}});
  EXPECT_EQ(parse_timestamp("1969-12-31T23:30:00.001-00:30", true),
            system_clock::time_point{milliseconds{1}});
  for (const auto* invalid : {"2026-07-12T15:21:04.Z", "2026-07-12T15:21:04.1234567890Z",
                              "2026-07-12T15:21:04+24:00", "2026-07-12T15:21:04-00:60"})
    EXPECT_EQ(parse_timestamp(invalid, true), std::nullopt) << invalid;
  if constexpr (system_clock::period::den == 1'000'000'000 && sizeof(system_clock::rep) == 8) {
    EXPECT_EQ(parse_timestamp("1677-09-21T00:12:43.145224192Z", true),
              system_clock::time_point::min());
    EXPECT_EQ(parse_timestamp("2262-04-11T23:47:16.854775808Z", true), std::nullopt);
  }
}
