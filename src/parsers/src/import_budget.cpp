#include "loginsight/parsers/import_budget.hpp"

#include <istream>
#include <stdexcept>

namespace loginsight::parsers {

void ImportBudget::checkCancelled() const {
  if (stop.stop_requested())
    throw std::runtime_error("import cancelled");
}

bool ImportBudget::readLine(std::istream& input, std::string& line) {
  line.clear();
  char chunk[4096];
  for (;;) {
    checkCancelled();
    input.getline(chunk, sizeof(chunk));
    const auto consumed = static_cast<std::size_t>(input.gcount());
    if (input.bad())
      return false;
    if (consumed == 0 && line.empty())
      return false;
    const bool more = input.fail() && !input.eof() && consumed == sizeof(chunk) - 1;
    const auto content = consumed - static_cast<std::size_t>(!input.fail() && !input.eof());
    if (consumed > maxBytes - bytes)
      throw std::runtime_error("session exceeds the 32 MiB import limit");
    if (content > maxLineBytes - line.size())
      throw std::runtime_error("a record exceeds the 256 KiB line limit");
    bytes += consumed;
    line.append(chunk, content);
    if (more) {
      input.clear(input.rdstate() & ~std::ios::failbit);
      continue;
    }
    if (++lines > maxLines)
      throw std::runtime_error("session exceeds the 200,000 line import limit");
    if (progress)
      progress(bytes);
    return true;
  }
}

} // namespace loginsight::parsers
