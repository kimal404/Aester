#include "features/cpufreq/cpufreq.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/state.hpp"
#include "core/sysfs.hpp"
#include "core/validate.hpp"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace aester {
namespace cpufreq {

namespace {

constexpr int VERIFY_MAX_RETRIES = 20;
constexpr useconds_t VERIFY_RETRY_DELAY_US = 25000; // 20 * 25 ms = 500 ms total

uint64_t parse_u64_safe(const std::string& s) {
    try {
        if (s.empty()) return 0;
        return std::stoull(s);
    } catch (...) {
        return 0;
    }
}

bool verify_with_retry(const std::string& path, uint64_t target_khz,
                       int retries = VERIFY_MAX_RETRIES,
                       useconds_t delay_us = VERIFY_RETRY_DELAY_US) {
    for (int i = 0; i < retries; ++i) {
        auto res = sysfs::read_text(path);
        if (res.ok()) {
            uint64_t actual = parse_u64_safe(res.value());
            if (actual > 0 && (actual == target_khz || actual <= target_khz)) {
                return true;
            }
        }
        usleep(delay_us);
    }
    return false;
}

} // namespace

Result<std::vector<std::string>> get_policies() {
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
    auto policies_res = get_policies();
    if (!policies_res.ok()) {
        return Result<std::string>::failure(policies_res.code(), policies_res.message());
    }
    const auto& policies = policies_res.value();

    const char* dr_env = std::getenv("AESTER_DRYRUN");
    bool is_dry_run = (dr_env != nullptr && dr_env[0] == '1');

    if (val_str == "off") {
        if (is_dry_run) {
            std::string targets_str;
            for (size_t i = 0; i < policies.size(); ++i) {
                targets_str += (i == 0 ? "" : ", ") + policies[i];
            }
            log::debug("dry-run cpufreq set target policies: " + targets_str);
            for (const auto& pol : policies) {
                std::string target_path = "devices/system/cpu/cpufreq/" + pol + "/scaling_max_freq";
                sysfs::write_text(target_path, "off");
            }
            return Result<std::string>::success("dry-run off");
        }

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

    if (!validate::is_valid_cpufreq_arg(val_str)) {
        return Result<std::string>::failure(ExitCode::InvalidArgs, "Nilai limit tidak valid (harus 40..90 atau off): " + val_str);
    }

    int percent = 0;
    try {
        percent = std::stoi(val_str);
    } catch (...) {
        return Result<std::string>::failure(ExitCode::InvalidArgs, "Format limit angka tidak valid: " + val_str);
    }

    // 1. Baca nilai sebelum penulisan untuk rollback
    std::vector<std::pair<std::string, std::string>> prev_values;
    for (const auto& pol : policies) {
        std::string cur_max_path = "devices/system/cpu/cpufreq/" + pol + "/scaling_max_freq";
        auto cur_res = sysfs::read_text(cur_max_path);
        prev_values.emplace_back(pol, cur_res.ok() ? cur_res.value() : "");
    }

    // 2. Periksa mode dry-run
    if (is_dry_run) {
        std::string targets_str;
        for (size_t i = 0; i < policies.size(); ++i) {
            targets_str += (i == 0 ? "" : ", ") + policies[i];
        }
        log::debug("dry-run cpufreq set target policies: " + targets_str);
        for (const auto& pol : policies) {
            std::string cur_max_path = "devices/system/cpu/cpufreq/" + pol + "/scaling_max_freq";
            sysfs::write_text(cur_max_path, val_str);
        }
        return Result<std::string>::success("dry-run " + val_str);
    }

    // 3. Simpan nilai asal ke orig state
    for (const auto& item : prev_values) {
        if (!item.second.empty()) {
            state::save_orig_value("cpufreq_" + item.first, item.second);
        }
    }

    // 4. Hitung target frekuensi untuk tiap policy
    std::vector<std::pair<std::string, uint64_t>> target_freqs;
    for (const auto& pol : policies) {
        std::string policy_path = "devices/system/cpu/cpufreq/" + pol;
        std::string cpuinfo_path = policy_path + "/cpuinfo_max_freq";
        std::string freqs_path = policy_path + "/scaling_available_frequencies";

        auto max_limit_res = sysfs::read_text(cpuinfo_path);
        if (!max_limit_res.ok()) {
            return Result<std::string>::failure(ExitCode::GeneralError, pol + ": gagal membaca cpuinfo_max_freq");
        }

        uint64_t max_limit = parse_u64_safe(max_limit_res.value());
        if (max_limit == 0) {
            return Result<std::string>::failure(ExitCode::GeneralError, pol + ": nilai cpuinfo_max_freq kosong");
        }

        uint64_t target_khz = (max_limit * percent) / 100;
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
        target_freqs.emplace_back(pol, target_khz);
    }

    // 5. Tulis ke SEMUA target policy terlebih dahulu
    std::vector<std::string> failed_policies;
    for (const auto& item : target_freqs) {
        std::string cur_max_path = "devices/system/cpu/cpufreq/" + item.first + "/scaling_max_freq";
        auto write_res = sysfs::write_text(cur_max_path, std::to_string(item.second));
        if (!write_res.ok()) {
            failed_policies.push_back(item.first);
        }
    }

    // 6. Baca ulang SEMUA target policy
    for (const auto& item : target_freqs) {
        if (std::find(failed_policies.begin(), failed_policies.end(), item.first) != failed_policies.end()) {
            continue;
        }
        std::string cur_max_path = "devices/system/cpu/cpufreq/" + item.first + "/scaling_max_freq";
        if (!verify_with_retry(cur_max_path, item.second)) {
            failed_policies.push_back(item.first);
        }
    }

    // 7. Jika ada kegagalan: pemulihan best-effort dan exit 4
    if (!failed_policies.empty()) {
        for (const auto& item : prev_values) {
            if (!item.second.empty()) {
                std::string cur_max_path = "devices/system/cpu/cpufreq/" + item.first + "/scaling_max_freq";
                sysfs::write_text(cur_max_path, item.second);
                state::remove_orig_value("cpufreq_" + item.first);
            }
        }
        std::string fail_list;
        for (size_t i = 0; i < failed_policies.size(); ++i) {
            fail_list += (i == 0 ? "" : ", ") + failed_policies[i];
        }
        return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed,
            "verifikasi cpufreq gagal pada target: " + fail_list);
    }

    state::set_config_value("cpufreq", val_str);
    return Result<std::string>::success(val_str);
}

} // namespace cpufreq
} // namespace aester
