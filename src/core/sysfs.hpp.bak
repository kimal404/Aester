#pragma once

#include "core/result.hpp"

#include <string>
#include <vector>

namespace aester {
namespace sysfs {

std::string resolve_path(const std::string& sysfs_path);
Result<std::string> read_text(const std::string& sysfs_path);
Result<void> write_text(const std::string& sysfs_path, const std::string& value);
Result<std::vector<std::string>> list_dir(const std::string& sysfs_path);
bool exists(const std::string& sysfs_path);

} // namespace sysfs
} // namespace aester
