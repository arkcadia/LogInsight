#pragma once

#include "loginsight/parsers/log_parser.hpp"

namespace loginsight::parsers {

struct CsvColumns {
  std::string timestamp{"timestamp"};
  std::string level{"level"};
  std::string source{"source"};
  std::string message{"message"};

  [[nodiscard]] bool valid() const;
};

class CsvLogParser final : public ILogParser {
public:
  explicit CsvLogParser(CsvColumns columns = {});
  [[nodiscard]] bool canParse(const std::filesystem::path& path) const override;
  [[nodiscard]] ParseResult parse(const std::filesystem::path& path) const override {
    return parse(path, nullptr);
  }
  [[nodiscard]] ParseResult parse(const std::filesystem::path& path,
                                  ImportBudget* budget) const override;

private:
  CsvColumns columns_;
};

} // namespace loginsight::parsers
