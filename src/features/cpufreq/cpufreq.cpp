#include "features/cpufreq/cpufreq.hpp"
#include "core/log.hpp"
#include "core/state.hpp"
#include "core/sysfs.hpp"
#include "core/validate.hpp"

#include <algorithm>
#include <sstream>

namespace aester {
namespace cpufreq {

std::vector<unsigned long> parse_frequencies(const std::string& text) {
    std::vector<unsigned long> freqs;
    std::istringstream iss(text);
    unsigned long val = 0;
    while (iss >> val) {
        freqs.push_back(val);
    }
    std::sort(freqs.begin(), freqs.end());
    return freqs;
}

unsigned long calculate_target_freq(unsigned long max_freq, int percent) {
    if (percent <= 0) return 0;
    return (max_freq * static_cast<unsigned long>(percent)) / 100UL;
}

unsigned long find_closest_freq(const std::vector<unsigned long>& available, unsigned long target) {
    if (available.empty()) return target;

    unsigned long best = available.front();
    for (unsigned long f : available) {
        if (f <= target) {
            best = f;
        } else {
            break;
        }
    }
    return best;
}

Result<std::vector<std::string>> get_policies() {
    auto res = sysfs::list_dir("devices/system/cpu/cpufreq");
    if (!res.ok()) {
        return Result<std::vector<std::string>>::failure(res.code(), "Gagal membaca direktori cpufreq: " + res.message());
    }

    std::vector<std::string> policies;
    for (const auto& entry : res.value()) {
        if (entry.rfind("policy", 0) == 0) {
            policies.push_back(entry);
        }
    }

    if (policies.empty()) {
        return Result<std::vector<std::string>>::failure(ExitCode::NotSupported, "Tidak ada cpufreq policy ditemukan");
    }

    std::sort(policies.begin(), policies.end());
    return Result<std::vector<std::string>>::success(policies);
}

Result<std::string> get_current_limit() {
    auto res = state::get_config_value("cpufreq");
    if (!res.ok()) {
        return Result<std::string>::success("off");
    }
    return Result<std::string>::success(res.value());
}

Result<std::string> set_limit(const std::string& percent_str) {
    if (!validate::is_valid_cpufreq_arg(percent_str)) {
        return Result<std::string>::failure(ExitCode::InvalidArgs, "Argumen cpufreq harus 40..90 atau off: " + percent_str);
    }

    auto pol_res = get_policies();
    if (!pol_res.ok()) return pol_res;
    const auto& policies = pol_res.value();

    if (percent_str == "off") {
        for (const auto& policy : policies) {
            std::string orig_key = "cpufreq_" + policy;
            auto orig_res = state::load_orig_value(orig_key);
            if (orig_res.ok()) {
                std::string target_path = "devices/system/cpu/cpufreq/" + policy + "/scaling_max_freq";
                auto write_res = sysfs::write_text(target_path, orig_res.value());
                if (!write_res.ok()) {
                    return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": gagal memulihkan frekuensi asli");
                }

                auto verify_res = sysfs::read_text(target_path);
                if (!verify_res.ok() || verify_res.value() != orig_res.value()) {
                    return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": verifikasi gagal setelah restore");
                }
                state::remove_orig_value(orig_key);
            }
        }
        state::set_config_value("cpufreq", "off");
        return Result<std::string>::success("off");
    }

    int percent = std::stoi(percent_str);

    for (const auto& policy : policies) {
        std::string pol_base = "devices/system/cpu/cpufreq/" + policy;

        auto cpuinfo_res = sysfs::read_text(pol_base + "/cpuinfo_max_freq");
        if (!cpuinfo_res.ok()) {
            return Result<std::string>::failure(ExitCode::GeneralError, policy + ": gagal membaca cpuinfo_max_freq");
        }
        unsigned long cpuinfo_max = std::stoul(cpuinfo_res.value());

        auto current_max_res = sysfs::read_text(pol_base + "/scaling_max_freq");
        if (current_max_res.ok()) {
            state::save_orig_value("cpufreq_" + policy, current_max_res.value());
        }

        unsigned long target = calculate_target_freq(cpuinfo_max, percent);
        unsigned long final_freq = target;

        auto avail_res = sysfs::read_text(pol_base + "/scaling_available_frequencies");
        if (avail_res.ok() && !avail_res.value().empty()) {
            auto freqs = parse_frequencies(avail_res.value());
            final_freq = find_closest_freq(freqs, target);
        } else {
            log::info(policy + ": scaling_available_frequencies tidak ditemukan, memakai target mentah");
        }

        std::string final_str = std::to_string(final_freq);
        std::string target_path = pol_base + "/scaling_max_freq";

        auto write_res = sysfs::write_text(target_path, final_str);
        if (!write_res.ok()) {
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": gagal menulis scaling_max_freq");
        }

        auto verify_res = sysfs::read_text(target_path);
        if (!verify_res.ok() || verify_res.value() != final_str) {
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": verifikasi tidak cocok setelah penulisan");
        }
    }

    state::set_config_value("cpufreq", percent_str);
    return Result<std::string>::success(percent_str);
}

} // namespace cpufreq
} // namespace aester
