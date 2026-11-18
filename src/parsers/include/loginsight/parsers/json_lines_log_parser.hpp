#pragma once

#include "loginsight/parsers/json_parser_config.hpp"
#include "loginsight/parsers/log_parser.hpp"

namespace loginsight::parsers {

class JsonLinesLogParser final : public ILogParser {
public:
  explicit JsonLinesLogParser(JsonParserConfig config = {});
  [[nodiscard]] bool canParse(const std::filesystem::path& path) const override;
  [[nodiscard]] ParseResult parse(const std::filesystem::path& path) const override;

private:
  JsonParserConfig config_;
};

} // namespace loginsight::parsers
