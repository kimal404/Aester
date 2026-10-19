#include "core/validate.hpp"

#include <algorithm>

namespace aester {
namespace validate {

bool is_valid_cpufreq_arg(const std::string& arg) {
    return (arg == "40" || arg == "50" || arg == "60" ||
            arg == "70" || arg == "80" || arg == "90" || arg == "off");
}

bool is_valid_identifier(const std::string& name) {
    if (name.empty()) return false;
    for (char c : name) {
        bool ok = (c >= 'a' && c <= 'z') ||
                  (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  (c == '_') || (c == '-');
        if (!ok) return false;
    }
    return true;
}

bool is_in_list(const std::string& item, const std::vector<std::string>& list) {
    return std::find(list.begin(), list.end(), item) != list.end();
}

std::vector<std::string> intersect_lists(const std::vector<std::vector<std::string>>& lists) {
    if (lists.empty()) return {};

    std::vector<std::string> result = lists[0];
    for (size_t i = 1; i < lists.size(); ++i) {
        std::vector<std::string> next_res;
        for (const auto& item : result) {
            if (is_in_list(item, lists[i])) {
                next_res.push_back(item);
            }
        }
        result = std::move(next_res);
    }
    return result;
}

} // namespace validate
} // namespace aester
