#pragma once

#include "loginsight/parsers/csv_log_parser.hpp"
#include "loginsight/parsers/json_parser_config.hpp"
#include "loginsight/parsers/log_parser.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace loginsight::parsers {

class ParserRegistry {
public:
  [[nodiscard]] bool registerParser(std::unique_ptr<ILogParser> parser);
  [[nodiscard]] const ILogParser* findParser(const std::filesystem::path& path) const;

private:
  std::vector<std::unique_ptr<ILogParser>> parsers_;
};

[[nodiscard]] ParserRegistry default_parser_registry(JsonParserConfig config = {},
                                                     CsvColumns csv = {});

} // namespace loginsight::parsers
