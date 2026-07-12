#pragma once

#include <string_view>

namespace loginsight::alerts {

[[nodiscard]] std::string_view module_name() noexcept;

} // namespace loginsight::alerts
