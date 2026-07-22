#include "loginsight/parsers/parser_registry.hpp"

#include <utility>

namespace loginsight::parsers {

bool ParserRegistry::registerParser(std::unique_ptr<ILogParser> parser) {
  if (parser == nullptr) {
    return false;
  }

  parsers_.push_back(std::move(parser));
  return true;
}

const ILogParser* ParserRegistry::findParser(const std::filesystem::path& path) const {
  for (const auto& parser : parsers_) {
    if (parser->canParse(path)) {
      return parser.get();
    }
  }

  return nullptr;
}

} // namespace loginsight::parsers
