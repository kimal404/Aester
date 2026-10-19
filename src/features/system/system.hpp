#pragma once

#include "core/result.hpp"

#include <string>

namespace aester {
namespace system {

Result<std::string> apply_saved();
Result<std::string> get_status_key_value();
Result<std::string> get_status_pretty();

} // namespace system
} // namespace aester
