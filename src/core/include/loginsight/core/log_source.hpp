#pragma once

#include <optional>
#include <string>

namespace loginsight::core {

struct LogSource {
  std::string name{};
  std::optional<std::string> host{};
  std::optional<std::string> application{};
  std::optional<std::string> file_path{};

  friend bool operator==(const LogSource&, const LogSource&) = default;
};

} // namespace loginsight::core
