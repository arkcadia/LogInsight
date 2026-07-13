#pragma once

#include "loginsight/parsers/log_parser.hpp"

namespace loginsight::parsers {

class PlainTextLogParser final : public ILogParser {
public:
  [[nodiscard]] ParseResult parse(std::istream& input,
                                  std::string_view source_name = {}) const override;
};

} // namespace loginsight::parsers
