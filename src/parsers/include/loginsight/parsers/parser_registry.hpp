#pragma once

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

} // namespace loginsight::parsers
