#pragma once

#include "loginsight/parsers/log_parser.hpp"

#include <filesystem>
#include <istream>

namespace loginsight::parsers {

// Plain text lines use: YYYY-MM-DD HH:MM:SS LEVEL SOURCE MESSAGE.
// UNKNOWN is reported as an unsupported level instead of becoming LogLevel::unknown.
class PlainTextLogParser final : public ILogParser {
public:
  [[nodiscard]] bool canParse(const std::filesystem::path& path) const override;
  [[nodiscard]] ParseResult parse(const std::filesystem::path& path) const override;
  [[nodiscard]] ParseResult parse(std::istream& input) const;
};

} // namespace loginsight::parsers
