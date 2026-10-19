#include "features/governor/governor.hpp"
#include "core/log.hpp"
#include "core/state.hpp"
#include "core/sysfs.hpp"
#include "core/validate.hpp"
#include "features/cpufreq/cpufreq.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <unistd.h>

namespace aester {
namespace governor {

namespace {

constexpr int VERIFY_MAX_RETRIES = 20;
constexpr useconds_t VERIFY_RETRY_DELAY_US = 25000; // 20 * 25 ms = 500 ms total

bool verify_with_retry(const std::string& path, const std::string& expected,
                       int retries = VERIFY_MAX_RETRIES,
                       useconds_t delay_us = VERIFY_RETRY_DELAY_US) {
    for (int i = 0; i < retries; ++i) {
        auto res = sysfs::read_text(path);
        if (res.ok() && res.value() == expected) {
            return true;
        }
        usleep(delay_us);
    }
    return false;
}

} // namespace

std::vector<std::string> parse_governors(const std::string& text) {
    std::vector<std::string> list;
    std::istringstream iss(text);
    std::string gov;
    while (iss >> gov) {
        list.push_back(gov);
    }
    return list;
}

Result<std::vector<std::string>> get_available() {
    auto pol_res = cpufreq::get_policies();
    if (!pol_res.ok()) {
        return Result<std::vector<std::string>>::failure(pol_res.code(), pol_res.message());
    }

    std::vector<std::vector<std::string>> all_govs;
    for (const auto& policy : pol_res.value()) {
        std::string path = "devices/system/cpu/cpufreq/" + policy + "/scaling_available_governors";
        auto res = sysfs::read_text(path);
        if (res.ok()) {
            all_govs.push_back(parse_governors(res.value()));
        }
    }

    if (all_govs.empty()) {
        return Result<std::vector<std::string>>::failure(ExitCode::NotSupported, "scaling_available_governors tidak ditemukan");
    }

    auto common = validate::intersect_lists(all_govs);
    if (common.empty()) {
        return Result<std::vector<std::string>>::failure(ExitCode::NotSupported, "Tidak ada governor umum yang didukung semua cluster");
    }

    return Result<std::vector<std::string>>::success(common);
}

Result<std::string> get_current() {
    auto saved = state::get_config_value("governor");
    if (saved.ok()) {
        return Result<std::string>::success(saved.value());
    }

    auto read_res = sysfs::read_text("devices/system/cpu/cpufreq/policy0/scaling_governor");
    if (!read_res.ok()) {
        return Result<std::string>::failure(ExitCode::GeneralError, "Gagal membaca scaling_governor policy0");
    }
    return Result<std::string>::success(read_res.value());
}

Result<std::string> set_governor(const std::string& name) {
    if (!validate::is_valid_identifier(name)) {
        return Result<std::string>::failure(ExitCode::InvalidArgs, "Nama governor tidak valid: " + name);
    }

    auto avail_res = get_available();
    if (!avail_res.ok()) {
        return Result<std::string>::failure(avail_res.code(), avail_res.message());
    }

    if (!validate::is_in_list(name, avail_res.value())) {
        return Result<std::string>::failure(ExitCode::InvalidArgs, "Governor tidak didukung perangkat: " + name);
    }

    auto pol_res = cpufreq::get_policies();
    if (!pol_res.ok()) {
        return Result<std::string>::failure(pol_res.code(), pol_res.message());
    }
    const auto& policies = pol_res.value();

    // 1. Catat nilai aktual sebelum penulisan untuk pemulihan (rollback)
    std::vector<std::pair<std::string, std::string>> prev_values;
    for (const auto& policy : policies) {
        std::string target_path = "devices/system/cpu/cpufreq/" + policy + "/scaling_governor";
        auto cur_res = sysfs::read_text(target_path);
        prev_values.emplace_back(policy, cur_res.ok() ? cur_res.value() : "");
    }

    // 2. Periksa mode dry-run
    const char* dr_env = std::getenv("AESTER_DRYRUN");
    bool is_dry_run = (dr_env != nullptr && dr_env[0] == '1');
    if (is_dry_run) {
        std::string targets_str;
        for (size_t i = 0; i < policies.size(); ++i) {
            targets_str += (i == 0 ? "" : ", ") + policies[i];
        }
        log::debug("dry-run governor set target policies: " + targets_str);
        for (const auto& policy : policies) {
            std::string target_path = "devices/system/cpu/cpufreq/" + policy + "/scaling_governor";
            sysfs::write_text(target_path, name);
        }
        return Result<std::string>::success("dry-run " + name);
    }

    // 3. Simpan nilai asal ke orig state sebelum mengubah sysfs
    for (const auto& item : prev_values) {
        if (!item.second.empty()) {
            state::save_orig_value("governor_" + item.first, item.second);
        }
    }

    // 4. Tulis ke SEMUA target dulu tanpa berhenti di kegagalan pertama
    std::vector<std::string> failed_targets;
    for (const auto& policy : policies) {
        std::string target_path = "devices/system/cpu/cpufreq/" + policy + "/scaling_governor";
        auto write_res = sysfs::write_text(target_path, name);
        if (!write_res.ok()) {
            failed_targets.push_back(policy);
        }
    }

    // 5. Baca ulang SEMUA target
    for (const auto& policy : policies) {
        if (std::find(failed_targets.begin(), failed_targets.end(), policy) != failed_targets.end()) {
            continue;
        }
        std::string target_path = "devices/system/cpu/cpufreq/" + policy + "/scaling_governor";
        if (!verify_with_retry(target_path, name)) {
            failed_targets.push_back(policy);
        }
    }

    // 6. Jika ada target yang gagal: pemulihan best-effort dan exit 4
    if (!failed_targets.empty()) {
        for (const auto& item : prev_values) {
            if (!item.second.empty()) {
                std::string target_path = "devices/system/cpu/cpufreq/" + item.first + "/scaling_governor";
                sysfs::write_text(target_path, item.second);
                state::remove_orig_value("governor_" + item.first);
            }
        }
        std::string fail_list;
        for (size_t i = 0; i < failed_targets.size(); ++i) {
            fail_list += (i == 0 ? "" : ", ") + failed_targets[i];
        }
        return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed,
            "verifikasi governor gagal pada target: " + fail_list);
    }

    state::set_config_value("governor", name);
    return Result<std::string>::success(name);
}

Result<std::string> reset() {
    auto pol_res = cpufreq::get_policies();
    if (!pol_res.ok()) {
        return Result<std::string>::failure(pol_res.code(), pol_res.message());
    }
    const auto& policies = pol_res.value();

    for (const auto& policy : policies) {
        std::string orig_key = "governor_" + policy;
        auto orig_res = state::load_orig_value(orig_key);
        if (orig_res.ok()) {
            std::string target_path = "devices/system/cpu/cpufreq/" + policy + "/scaling_governor";
            auto write_res = sysfs::write_text(target_path, orig_res.value());
            if (!write_res.ok()) {
                return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": gagal memulihkan governor asli");
            }

            if (!verify_with_retry(target_path, orig_res.value())) {
                auto cur = sysfs::read_text(target_path);
                std::string actual = cur.ok() ? cur.value() : "unknown";
                return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": verifikasi restore gagal (target: " + orig_res.value() + ", aktual: " + actual + ")");
            }
            state::remove_orig_value(orig_key);
        }
    }

    state::remove_config_value("governor");
    return get_current();
}

} // namespace governor
} // namespace aester
