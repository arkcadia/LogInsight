#pragma once

#include <string_view>

namespace loginsight::storage {

[[nodiscard]] std::string_view module_name() noexcept;

} // namespace loginsight::storage
