#pragma once

#include <string>
#include <vector>

namespace aester {
namespace validate {

bool is_valid_cpufreq_arg(const std::string& arg);
bool is_valid_identifier(const std::string& name);
bool is_in_list(const std::string& item, const std::vector<std::string>& list);
std::vector<std::string> intersect_lists(const std::vector<std::vector<std::string>>& lists);

} // namespace validate
} // namespace aester
