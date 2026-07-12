#pragma once

#include <string_view>

namespace loginsight::core {

[[nodiscard]] std::string_view project_name() noexcept;
[[nodiscard]] std::string_view version() noexcept;

} // namespace loginsight::core
