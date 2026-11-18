#include "loginsight/parsers/parser_registry.hpp"

#include "loginsight/parsers/json_lines_log_parser.hpp"
#include "loginsight/parsers/plain_text_log_parser.hpp"

#include <stdexcept>
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

ParserRegistry default_parser_registry() {
  ParserRegistry registry;
  if (!registry.registerParser(std::make_unique<PlainTextLogParser>()) ||
      !registry.registerParser(std::make_unique<JsonLinesLogParser>()))
    throw std::runtime_error("unable to configure parser registry");
  return registry;
}

} // namespace loginsight::parsers
