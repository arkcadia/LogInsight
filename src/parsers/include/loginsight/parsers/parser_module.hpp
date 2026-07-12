#pragma once

#include <string_view>

namespace loginsight::parsers {

[[nodiscard]] std::string_view module_name() noexcept;

} // namespace loginsight::parsers
