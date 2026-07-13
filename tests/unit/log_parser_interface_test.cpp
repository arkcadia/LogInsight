#include "loginsight/parsers/log_parser.hpp"

#include <filesystem>

#include <gtest/gtest.h>

using loginsight::parsers::ILogParser;
using loginsight::parsers::ParseResult;

namespace {

class TestLogParser final : public ILogParser {
public:
  [[nodiscard]] bool canParse(const std::filesystem::path& path) const override {
    return path.extension() == ".testlog";
  }

  [[nodiscard]] ParseResult parse(const std::filesystem::path&) const override { return {}; }
};

} // namespace

TEST(LogParserInterfaceTest, SupportsPathBasedVirtualDispatch) {
  const TestLogParser parser;
  const ILogParser& interface = parser;

  EXPECT_TRUE(interface.canParse("example.testlog"));
  EXPECT_FALSE(interface.canParse("example.json"));
  EXPECT_TRUE(interface.parse("example.testlog").entries.empty());
  EXPECT_TRUE(interface.parse("example.testlog").errors.empty());
}
