#pragma once

#include <string_view>

namespace loginsight::rules {

[[nodiscard]] std::string_view module_name() noexcept;

} // namespace loginsight::rules
