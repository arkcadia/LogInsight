#include "loginsight/parsers/file_import_service.hpp"

#include "loginsight/parsers/log_parser.hpp"
#include "loginsight/parsers/parser_registry.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

using loginsight::parsers::FileImportService;
using loginsight::parsers::FileImportStatus;
using loginsight::parsers::ILogParser;
using loginsight::parsers::ParseError;
using loginsight::parsers::ParseResult;
using loginsight::parsers::ParserRegistry;

namespace {

class FakeParser final : public ILogParser {
public:
  FakeParser(bool can_parse, ParseResult parse_result)
      : can_parse_{can_parse}, parse_result_{std::move(parse_result)} {}

  [[nodiscard]] bool canParse(const std::filesystem::path&) const override { return can_parse_; }

  [[nodiscard]] ParseResult parse(const std::filesystem::path& path) const override {
    ++parse_call_count_;
    parsed_path_ = path;
    return parse_result_;
  }

  [[nodiscard]] std::size_t parseCallCount() const noexcept { return parse_call_count_; }
  [[nodiscard]] const std::filesystem::path& parsedPath() const noexcept { return parsed_path_; }

private:
  bool can_parse_;
  ParseResult parse_result_;
  mutable std::size_t parse_call_count_{};
  mutable std::filesystem::path parsed_path_;
};

} // namespace

TEST(FileImportServiceTest, SelectsAParserAndReturnsItsParseResult) {
  ParserRegistry registry;
  const auto path = std::filesystem::path{"application.log"};
  auto parser = std::make_unique<FakeParser>(true, ParseResult{.entries = {},
                                                               .errors = {ParseError{
                                                                   .lineNumber = 3U,
                                                                   .rawContent = "malformed entry",
                                                                   .message = "invalid timestamp",
                                                               }}});
  const auto* parser_view = parser.get();
  ASSERT_TRUE(registry.registerParser(std::move(parser)));

  const FileImportService service{registry};
  const auto result = service.importFile(path);

  EXPECT_EQ(result.status, FileImportStatus::completed);
  EXPECT_TRUE(result.isSupported());
  ASSERT_EQ(result.parseResult.errors.size(), 1U);
  EXPECT_EQ(result.parseResult.errors.front().lineNumber, 3U);
  EXPECT_EQ(result.parseResult.errors.front().message, "invalid timestamp");
  EXPECT_EQ(parser_view->parseCallCount(), 1U);
  EXPECT_EQ(parser_view->parsedPath(), path);
}

TEST(FileImportServiceTest, ReportsUnsupportedFormatWithoutExecutingAParser) {
  ParserRegistry registry;
  auto parser = std::make_unique<FakeParser>(false, ParseResult{});
  const auto* parser_view = parser.get();
  ASSERT_TRUE(registry.registerParser(std::move(parser)));

  const FileImportService service{registry};
  const auto result = service.importFile("application.unknown");

  EXPECT_EQ(result.status, FileImportStatus::unsupported_format);
  EXPECT_FALSE(result.isSupported());
  EXPECT_TRUE(result.parseResult.entries.empty());
  EXPECT_FALSE(result.parseResult.has_errors());
  EXPECT_EQ(parser_view->parseCallCount(), 0U);
}

TEST(FileImportServiceTest, ExecutesOnlyTheFirstCompatibleParser) {
  ParserRegistry registry;
  auto first_parser = std::make_unique<FakeParser>(true, ParseResult{});
  const auto* first_parser_view = first_parser.get();
  auto second_parser = std::make_unique<FakeParser>(true, ParseResult{});
  const auto* second_parser_view = second_parser.get();
  ASSERT_TRUE(registry.registerParser(std::move(first_parser)));
  ASSERT_TRUE(registry.registerParser(std::move(second_parser)));

  const FileImportService service{registry};
  const auto result = service.importFile("application.log");

  EXPECT_EQ(result.status, FileImportStatus::completed);
  EXPECT_EQ(first_parser_view->parseCallCount(), 1U);
  EXPECT_EQ(second_parser_view->parseCallCount(), 0U);
}
