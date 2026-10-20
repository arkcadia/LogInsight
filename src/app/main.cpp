#include "loginsight/app/analysis_command.hpp"

#include "loginsight/parsers/file_import_service.hpp"
#include "loginsight/parsers/parser_registry.hpp"
#include "loginsight/parsers/plain_text_log_parser.hpp"

#include <cstddef>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) try {
  loginsight::parsers::ParserRegistry parser_registry;
  if (!parser_registry.registerParser(
          std::make_unique<loginsight::parsers::PlainTextLogParser>())) {
    std::cerr << "Error: unable to configure parser registry.\n";
    return static_cast<int>(loginsight::app::CommandExitCode::configuration_failure);
  }

  const loginsight::parsers::FileImportService import_service{parser_registry};

  std::vector<std::string_view> arguments;
  arguments.reserve(static_cast<std::size_t>(argc));
  for (int index = 0; index < argc; ++index) {
    arguments.emplace_back(argv[index]);
  }

  return loginsight::app::run_cli(arguments, import_service, std::cout, std::cerr);
}

catch (const std::exception& error) {
  std::cerr << "Error: " << error.what() << '\n';
  return static_cast<int>(loginsight::app::CommandExitCode::configuration_failure);
}
