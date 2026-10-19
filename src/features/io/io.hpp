#pragma once

#include "core/result.hpp"

#include <string>
#include <vector>

namespace aester {
namespace io {

struct SchedInfo {
    std::string current;
    std::vector<std::string> available;
};

// Logika murni parser scheduler sysfs
SchedInfo parse_scheduler_line(const std::string& line);

// Sysfs & State I/O
Result<std::vector<std::string>> get_block_devices();
Result<std::vector<std::string>> get_available();
Result<std::string> get_current();
Result<std::string> set_scheduler(const std::string& name);
Result<std::string> reset();

} // namespace io
} // namespace aester
