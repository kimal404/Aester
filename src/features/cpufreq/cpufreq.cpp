#include "features/cpufreq/cpufreq.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/state.hpp"
#include "core/sysfs.hpp"
#include "core/validate.hpp"

#include <algorithm>
#include <dirent.h>
#include <sstream>
#include <unistd.h>

namespace aester {
namespace cpufreq {

namespace {

bool verify_with_retry(const std::string& path, uint64_t target_khz, int retries = 10, useconds_t delay_us = 25000) {
    for (int i = 0; i < retries; ++i) {
        usleep(delay_us);
        auto res = sysfs::read_u64(path);
        // Valid jika nilai cocok atau terpotong thermal driver PPM (aktual <= target)
        if (res.ok() && res.value() > 0 && (res.value() == target_khz || res.value() <= target_khz)) {
            return true;
        }
    }
    return false;
}

} // namespace

Result<std::vector<std::string>> get_available_policies() {
    std::string cpufreq_root = paths::get_sysfs_root() + "/devices/system/cpu/cpufreq";
    DIR* dir = ::opendir(cpufreq_root.c_str());
    if (!dir) {
        return Result<std::vector<std::string>>::failure(ExitCode::GeneralError, "Gagal membuka direktori cpufreq sysfs");
    }

    std::vector<std::string> policies;
    struct dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.rfind("policy", 0) == 0) {
            policies.push_back(name);
        }
    }
    ::closedir(dir);

    if (policies.empty()) {
        return Result<std::vector<std::string>>::failure(ExitCode::NotSupported, "Tidak ada cpufreq policy yang ditemukan");
    }

    std::sort(policies.begin(), policies.end());
    return Result<std::vector<std::string>>::success(policies);
}

Result<std::string> get_current_limit() {
    auto saved = state::get_config_value("cpufreq");
    if (saved.ok()) {
        return Result<std::string>::success(saved.value());
    }
    return Result<std::string>::success("off");
}

Result<std::string> set_limit(const std::string& val_str) {
    auto policies_res = get_available_policies();
    if (!policies_res.ok()) {
        return Result<std::string>::failure(policies_res.code(), policies_res.message());
    }

    const auto& policies = policies_res.value();

    if (val_str == "off") {
        for (const auto& pol : policies) {
            std::string orig_key = "cpufreq_" + pol;
            auto orig_res = state::load_orig_value(orig_key);
            std::string max_freq_str;

            if (orig_res.ok()) {
                max_freq_str = orig_res.value();
            } else {
                auto cpuinfo_max = sysfs::read_text("devices/system/cpu/cpufreq/" + pol + "/cpuinfo_max_freq");
                if (cpuinfo_max.ok()) {
                    max_freq_str = cpuinfo_max.value();
                }
            }

            if (!max_freq_str.empty()) {
                std::string target_path = "devices/system/cpu/cpufreq/" + pol + "/scaling_max_freq";
                sysfs::write_text(target_path, max_freq_str);
                state::remove_orig_value(orig_key);
            }
        }
        state::remove_config_value("cpufreq");
        return Result<std::string>::success("off");
    }

    auto pct_res = validate::validate_percentage(val_str);
    if (!pct_res.ok()) {
        return Result<std::string>::failure(pct_res.code(), pct_res.message());
    }
    int percent = pct_res.value();

    for (const auto& pol : policies) {
        std::string policy_path = "devices/system/cpu/cpufreq/" + pol;
        std::string cur_max_path = policy_path + "/scaling_max_freq";
        std::string cpuinfo_path = policy_path + "/cpuinfo_max_freq";
        std::string freqs_path = policy_path + "/scaling_available_frequencies";

        auto cur_max = sysfs::read_text(cur_max_path);
        if (cur_max.ok()) {
            state::save_orig_value("cpufreq_" + pol, cur_max.value());
        }

        auto max_limit_res = sysfs::read_u64(cpuinfo_path);
        if (!max_limit_res.ok()) {
            return Result<std::string>::failure(ExitCode::GeneralError, pol + ": gagal membaca cpuinfo_max_freq");
        }

        uint64_t target_khz = (max_limit_res.value() * percent) / 100;

        auto avail_res = sysfs::read_text(freqs_path);
        if (avail_res.ok()) {
            std::istringstream iss(avail_res.value());
            uint64_t f;
            uint64_t best_match = 0;
            while (iss >> f) {
                if (f <= target_khz && f > best_match) {
                    best_match = f;
                }
            }
            if (best_match > 0) {
                target_khz = best_match;
            }
        }

        auto write_res = sysfs::write_text(cur_max_path, std::to_string(target_khz));
        if (!write_res.ok()) {
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, pol + ": gagal menulis scaling_max_freq");
        }

        if (!verify_with_retry(cur_max_path, target_khz)) {
            auto actual = sysfs::read_u64(cur_max_path);
            std::string act_str = actual.ok() ? std::to_string(actual.value()) : "unknown";
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, pol + ": verifikasi gagal (target: " + std::to_string(target_khz) + ", aktual: " + act_str + ")");
        }
    }

    state::set_config_value("cpufreq", val_str);
    return Result<std::string>::success(val_str);
}

} // namespace cpufreq
} // namespace aester
