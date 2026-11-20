#include "loginsight/parsers/json_lines_log_parser.hpp"
#include "loginsight/parsers/timestamp_parser.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace loginsight::parsers {
namespace {
using Json = nlohmann::json;

const std::string* find_string(const Json& record, std::string_view path) {
  const auto* value = &record;
  while (true) {
    const auto dot = path.find('.');
    const auto member = value->find(path.substr(0, dot));
    if (member == value->end())
      return nullptr;
    value = &*member;
    if (dot == std::string_view::npos)
      return value->get_ptr<const std::string*>();
    path.remove_prefix(dot + 1);
  }
}
} // namespace

JsonLinesLogParser::JsonLinesLogParser(JsonParserConfig config) : config_(std::move(config)) {
  if (config_.inputMode != JsonInputMode::json_lines || validate_json_parser_config(config_))
    throw std::invalid_argument("invalid JSON Lines parser configuration");
}

bool JsonLinesLogParser::canParse(const std::filesystem::path& path) const {
  auto extension = path.extension().string();
  for (auto& ch : extension) {
    if (ch >= 'A' && ch <= 'Z')
      ch = static_cast<char>(ch - 'A' + 'a');
  }
  return extension == ".jsonl" || extension == ".ndjson";
}

ParseResult JsonLinesLogParser::parse(const std::filesystem::path& path,
                                      ImportBudget* budget) const {
  std::ifstream input{path};
  if (!input)
    return {.errors = {{0U, path.string(), "unable to open file"}}};

  ParseResult result;
  std::string line;
  std::size_t line_number{};
  while (budget ? budget->readLine(input, line) : static_cast<bool>(std::getline(input, line))) {
    ++line_number;
    if (line.find_first_not_of(" \t\r") == std::string::npos)
      continue;
    if (line.back() == '\r')
      line.pop_back();

    const auto record = Json::parse(line, nullptr, false);
    const auto* timestamp_text = find_string(record, config_.timestampFieldPath);
    const auto* level_text = find_string(record, config_.levelFieldPath);
    const auto* message = find_string(record, config_.messageFieldPath);
    const auto* source =
        config_.sourceFieldPath ? find_string(record, *config_.sourceFieldPath) : nullptr;
    const auto timestamp = timestamp_text ? parse_timestamp(*timestamp_text, true) : std::nullopt;
    const auto level =
        core::log_level_from_string(level_text ? std::string_view{*level_text} : std::string_view{})
            .value_or(core::LogLevel::unknown);

    const char* error = nullptr;
    if (!record.is_object())
      error = "expected a JSON object";
    else if (!timestamp)
      error = "missing or invalid timestamp";
    else if (level == core::LogLevel::unknown)
      error = "missing or invalid severity";
    else if (config_.sourceFieldPath &&
             (!source || source->find_first_not_of(" \t\r\n") == std::string::npos))
      error = "missing or invalid source";
    else if (!message || message->find_first_not_of(" \t\r\n") == std::string::npos)
      error = "missing or invalid message";

    if (error) {
      collect_malformed(result, budget, {line_number, std::move(line), error});
      continue;
    }
    core::LogAttributes attributes;
    for (const auto* key : {"trace_id", "request_id"}) {
      if (const auto* id = find_string(record, key); id && !id->empty())
        attributes.emplace(key, *id);
    }
    collect_entry(result, budget,
                  {.timestamp = *timestamp,
                   .level = level,
                   .source = core::LogSource{.name = source ? *source : path.stem().string()},
                   .message = *message,
                   .rawContent = std::move(line),
                   .attributes = std::move(attributes)});
  }
  if (input.bad() || (input.fail() && !input.eof()))
    result.errors.push_back({0U, {}, "unable to read input"});
  return result;
}

} // namespace loginsight::parsers
