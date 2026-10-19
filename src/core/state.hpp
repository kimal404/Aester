#pragma once

#include "core/result.hpp"

#include <map>
#include <string>

namespace aester {
namespace state {

Result<void> init_dirs();
Result<std::map<std::string, std::string>> load_config();
Result<std::string> get_config_value(const std::string& key);
Result<void> set_config_value(const std::string& key, const std::string& value);
Result<void> remove_config_value(const std::string& key);

Result<void> save_orig_value(const std::string& name, const std::string& value);
Result<std::string> load_orig_value(const std::string& name);
bool has_orig_value(const std::string& name);
Result<void> remove_orig_value(const std::string& name);

} // namespace state
} // namespace aester
