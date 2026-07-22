#pragma once

#include "loginsight/parsers/file_import_service.hpp"

#include <iosfwd>
#include <span>
#include <string_view>

namespace loginsight::app {

enum class CommandExitCode : int {
  success = 0,
  invalid_arguments = 64,
  unsupported_format = 65,
  file_not_found = 66,
  file_open_failure = 74,
  configuration_failure = 78,
};

[[nodiscard]] int run_cli(std::span<const std::string_view> arguments,
                          const parsers::FileImportService& import_service, std::ostream& output,
                          std::ostream& error_output);

} // namespace loginsight::app
