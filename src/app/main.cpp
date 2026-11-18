#include "loginsight/app/analysis_command.hpp"

#include "loginsight/parsers/file_import_service.hpp"
#include "loginsight/parsers/parser_registry.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) try {
  const auto parser_registry = loginsight::parsers::default_parser_registry();
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
