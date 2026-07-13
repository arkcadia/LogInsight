#pragma once

#include "loginsight/parsers/log_parser.hpp"

#include <filesystem>
#include <istream>
#include <string_view>

namespace loginsight::parsers {

class PlainTextLogParser final : public ILogParser {
public:
  [[nodiscard]] bool canParse(const std::filesystem::path& path) const override;
  [[nodiscard]] ParseResult parse(const std::filesystem::path& path) const override;
  [[nodiscard]] ParseResult parse(std::istream& input, std::string_view source_name = {}) const;
};

} // namespace loginsight::parsers
