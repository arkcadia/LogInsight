#include "loginsight/parsers/csv_log_parser.hpp"
#include "loginsight/parsers/timestamp_parser.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <utility>

namespace loginsight::parsers {
namespace {

std::optional<std::vector<std::string>> read_columns(std::string_view line) {
  // shortcut: quoted fields cannot span physical lines, extend the reader if multiline logs are
  // needed.
  std::vector<std::string> fields;
  for (;;) {
    std::string value;
    if (line.starts_with('"')) {
      line.remove_prefix(1);
      for (;;) {
        const auto quote = line.find('"');
        if (quote == std::string_view::npos)
          return std::nullopt;
        value.append(line.substr(0, quote));
        line.remove_prefix(quote + 1);
        if (!line.starts_with('"'))
          break;
        value.push_back('"');
        line.remove_prefix(1);
      }
      if (!line.empty() && !line.starts_with(','))
        return std::nullopt;
    } else {
      const auto field = line.substr(0, line.find(','));
      if (field.find('"') != std::string_view::npos)
        return std::nullopt;
      value = field;
      line.remove_prefix(field.size());
    }
    fields.push_back(std::move(value));
    if (line.empty())
      return fields;
    line.remove_prefix(1);
  }
}

} // namespace

bool CsvColumns::valid() const {
  return !timestamp.empty() && !level.empty() && !message.empty() && timestamp != level &&
         timestamp != message && level != message && source != timestamp && source != level &&
         source != message;
}

CsvLogParser::CsvLogParser(CsvColumns columns) : columns_(std::move(columns)) {
  if (!columns_.valid())
    throw std::invalid_argument("CSV columns must be distinct; only source can be blank");
}

bool CsvLogParser::canParse(const std::filesystem::path& path) const {
  auto extension = path.extension().string();
  for (auto& ch : extension) {
    if (ch >= 'A' && ch <= 'Z')
      ch = static_cast<char>(ch - 'A' + 'a');
  }
  return extension == ".csv";
}

ParseResult CsvLogParser::parse(const std::filesystem::path& path, ImportBudget* budget) const {
  std::ifstream input{path};
  if (!input)
    return {.errors = {{0U, path.string(), "unable to open file"}}};
  std::string line;
  const auto next = [&] {
    const bool read =
        budget ? budget->readLine(input, line) : static_cast<bool>(std::getline(input, line));
    if (read && line.ends_with('\r'))
      line.pop_back();
    return read;
  };
  if (!next()) {
    if (input.bad() || !input.eof())
      return {.errors = {{0U, {}, "unable to read input"}}};
    return {.errors = {{1U, {}, "missing CSV header"}}};
  }

  auto headerText = std::string_view{line};
  if (headerText.starts_with("\xef\xbb\xbf"))
    headerText.remove_prefix(3);
  const auto header = read_columns(headerText);
  if (!header)
    return {.errors = {{1U, line, "malformed CSV header"}}};
  std::array<std::size_t, 4> indices{};
  const std::array names{&columns_.timestamp, &columns_.level, &columns_.source, &columns_.message};
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i]->empty())
      continue;
    const auto position = std::find(header->begin(), header->end(), *names[i]);
    if (position == header->end() ||
        std::find(position + 1, header->end(), *names[i]) != header->end())
      return {.errors = {{1U, line, "missing or duplicate CSV column: " + *names[i]}}};
    indices[i] = static_cast<std::size_t>(position - header->begin());
  }

  ParseResult result;
  std::size_t lineNumber = 1;
  while (next()) {
    ++lineNumber;
    if (line.empty())
      continue;
    const auto fields = read_columns(line);
    if (!fields || fields->size() != header->size()) {
      collect_malformed(result, budget,
                        {lineNumber, std::move(line), "malformed CSV row or wrong column count"});
      continue;
    }
    const auto& values = *fields;
    const auto timestamp = parse_timestamp(values[indices[0]], true);
    const auto level = core::log_level_from_string(values[indices[1]]);
    const auto& message = values[indices[3]];
    if (!timestamp || !level || *level == core::LogLevel::unknown ||
        message.find_first_not_of(" \t\r\n") == std::string::npos) {
      collect_malformed(result, budget,
                        {lineNumber, std::move(line),
                         !timestamp                                    ? "invalid timestamp"
                         : !level || *level == core::LogLevel::unknown ? "invalid severity"
                                                                       : "missing message"});
      continue;
    }
    collect_entry(
        result, budget,
        {.timestamp = *timestamp,
         .level = *level,
         .source = core::LogSource{.name = columns_.source.empty() || values[indices[2]].empty()
                                               ? path.stem().string()
                                               : values[indices[2]]},
         .message = message,
         .rawContent = std::move(line)});
  }
  if (input.bad() || (input.fail() && !input.eof()))
    result.errors.push_back({0U, {}, "unable to read input"});
  return result;
}

} // namespace loginsight::parsers
