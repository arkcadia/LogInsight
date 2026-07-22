#include "loginsight/parsers/parser_registry.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

using loginsight::parsers::ILogParser;
using loginsight::parsers::ParseResult;
using loginsight::parsers::ParserRegistry;

namespace {

class FakeParser final : public ILogParser {
public:
  explicit FakeParser(std::string extension) : extension_{std::move(extension)} {}

  [[nodiscard]] bool canParse(const std::filesystem::path& path) const override {
    return path.extension() == extension_;
  }

  [[nodiscard]] ParseResult parse(const std::filesystem::path&) const override { return {}; }

private:
  std::string extension_;
};

} // namespace

TEST(ParserRegistryTest, ReturnsMatchingParser) {
  ParserRegistry registry;
  auto parser = std::make_unique<FakeParser>(".log");
  const auto* parser_view = parser.get();

  ASSERT_TRUE(registry.registerParser(std::move(parser)));

  EXPECT_EQ(registry.findParser("application.log"), parser_view);
}

TEST(ParserRegistryTest, ReturnsNullWhenNoParserMatches) {
  ParserRegistry registry;
  ASSERT_TRUE(registry.registerParser(std::make_unique<FakeParser>(".log")));

  EXPECT_EQ(registry.findParser("events.json"), nullptr);
}

TEST(ParserRegistryTest, PreservesRegistrationOrderForMultipleParsers) {
  ParserRegistry registry;
  auto first_parser = std::make_unique<FakeParser>(".log");
  auto second_parser = std::make_unique<FakeParser>(".log");
  const auto* first_parser_view = first_parser.get();

  ASSERT_TRUE(registry.registerParser(std::move(first_parser)));
  ASSERT_TRUE(registry.registerParser(std::move(second_parser)));

  EXPECT_EQ(registry.findParser("application.log"), first_parser_view);
}

TEST(ParserRegistryTest, CanMatchLaterParserWhenEarlierParserDoesNotMatch) {
  ParserRegistry registry;
  auto json_parser = std::make_unique<FakeParser>(".json");
  const auto* json_parser_view = json_parser.get();

  ASSERT_TRUE(registry.registerParser(std::make_unique<FakeParser>(".log")));
  ASSERT_TRUE(registry.registerParser(std::move(json_parser)));

  EXPECT_EQ(registry.findParser("events.json"), json_parser_view);
}

TEST(ParserRegistryTest, RejectsNullParserRegistration) {
  ParserRegistry registry;

  EXPECT_FALSE(registry.registerParser(nullptr));
  EXPECT_EQ(registry.findParser("application.log"), nullptr);
}
