#include "features/governor/governor.hpp"
#include "core/log.hpp"
#include "core/state.hpp"
#include "core/sysfs.hpp"
#include "core/validate.hpp"
#include "features/cpufreq/cpufreq.hpp"

#include <sstream>
#include <unistd.h>

namespace aester {
namespace governor {

namespace {

bool verify_with_retry(const std::string& path, const std::string& expected, int retries = 10, useconds_t delay_us = 25000) {
    for (int i = 0; i < retries; ++i) {
        usleep(delay_us);
        auto res = sysfs::read_text(path);
        if (res.ok() && res.value() == expected) {
            return true;
        }
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

    // Jika belum pernah disetel lewat aesterctl, baca governor aktif policy0
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

    for (const auto& policy : policies) {
        std::string pol_base = "devices/system/cpu/cpufreq/" + policy;
        std::string target_path = pol_base + "/scaling_governor";

        auto current_res = sysfs::read_text(target_path);
        if (current_res.ok()) {
            state::save_orig_value("governor_" + policy, current_res.value());
        }

        auto write_res = sysfs::write_text(target_path, name);
        if (!write_res.ok()) {
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": gagal menulis scaling_governor");
        }

        if (!verify_with_retry(target_path, name)) {
            auto cur = sysfs::read_text(target_path);
            std::string actual = cur.ok() ? cur.value() : "unknown";
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, policy + ": verifikasi governor gagal (target: " + name + ", aktual: " + actual + ")");
        }
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
