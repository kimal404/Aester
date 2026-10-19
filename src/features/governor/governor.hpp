#pragma once

#include "core/result.hpp"

#include <string>
#include <vector>

namespace aester {
namespace governor {

// Logika murni
std::vector<std::string> parse_governors(const std::string& text);

// Sysfs & State
Result<std::vector<std::string>> get_available();
Result<std::string> get_current();
Result<std::string> set_governor(const std::string& name);
Result<std::string> reset();

} // namespace governor
} // namespace aester
