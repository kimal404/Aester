#pragma once

#include "core/result.hpp"

#include <string>
#include <vector>

namespace aester {
namespace cpufreq {

// Fungsi logika murni (tanpa sysfs I/O)
std::vector<unsigned long> parse_frequencies(const std::string& text);
unsigned long calculate_target_freq(unsigned long max_freq, int percent);
unsigned long find_closest_freq(const std::vector<unsigned long>& available, unsigned long target);

// Fungsi manipulasi sysfs & state
Result<std::vector<std::string>> get_policies();
Result<std::string> get_current_limit();
Result<std::string> set_limit(const std::string& percent_str);

} // namespace cpufreq
} // namespace aester
