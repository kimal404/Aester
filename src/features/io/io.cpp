#include "features/io/io.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/state.hpp"
#include "core/sysfs.hpp"
#include "core/validate.hpp"

#include <dirent.h>
#include <sstream>
#include <unistd.h>

namespace aester {
namespace io {

namespace {

bool verify_with_retry(const std::string& path, const std::string& expected, int retries = 10, useconds_t delay_us = 25000) {
    for (int i = 0; i < retries; ++i) {
        usleep(delay_us);
        auto res = sysfs::read_text(path);
        if (res.ok()) {
            auto info = parse_scheduler_line(res.value());
            if (info.current == expected) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

SchedInfo parse_scheduler_line(const std::string& line) {
    SchedInfo info;
    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
        if (!token.empty() && token.front() == '[' && token.back() == ']') {
            std::string name = token.substr(1, token.size() - 2);
            info.current = name;
            info.available.push_back(name);
        } else {
            info.available.push_back(token);
        }
    }
    return info;
}

Result<std::vector<std::string>> get_block_devices() {
    std::string block_root = paths::get_sysfs_root() + "/block";
    DIR* dir = ::opendir(block_root.c_str());
    if (!dir) {
        return Result<std::vector<std::string>>::failure(ExitCode::GeneralError, "Gagal membuka direktori /sys/block");
    }

    std::vector<std::string> devices;
    struct dirent* entry;
    while ((entry = ::readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        if (name.rfind("loop", 0) == 0) continue;
        if (name.rfind("ram", 0) == 0) continue;
        if (name.rfind("zram", 0) == 0) continue;
        if (name.rfind("dm-", 0) == 0) continue;

        std::string sched_rel = "block/" + name + "/queue/scheduler";
        if (sysfs::exists(sched_rel)) {
            devices.push_back(name);
        }
    }
    ::closedir(dir);

    if (devices.empty()) {
        return Result<std::vector<std::string>>::failure(ExitCode::NotSupported, "Tidak ada block queue scheduler yang ditemukan");
    }

    return Result<std::vector<std::string>>::success(devices);
}

Result<std::vector<std::string>> get_available() {
    auto dev_res = get_block_devices();
    if (!dev_res.ok()) {
        return Result<std::vector<std::string>>::failure(dev_res.code(), dev_res.message());
    }

    std::vector<std::vector<std::string>> all_scheds;
    for (const auto& dev : dev_res.value()) {
        auto read_res = sysfs::read_text("block/" + dev + "/queue/scheduler");
        if (read_res.ok()) {
            auto info = parse_scheduler_line(read_res.value());
            if (!info.available.empty()) {
                all_scheds.push_back(info.available);
            }
        }
    }

    if (all_scheds.empty()) {
        return Result<std::vector<std::string>>::failure(ExitCode::NotSupported, "Tidak ada scheduler yang terbaca");
    }

    auto common = validate::intersect_lists(all_scheds);
    if (common.empty()) {
        return Result<std::vector<std::string>>::failure(ExitCode::NotSupported, "Tidak ada scheduler umum untuk semua block device");
    }

    return Result<std::vector<std::string>>::success(common);
}

Result<std::string> get_current() {
    auto saved = state::get_config_value("io_scheduler");
    if (saved.ok()) {
        return Result<std::string>::success(saved.value());
    }

    auto dev_res = get_block_devices();
    if (!dev_res.ok() || dev_res.value().empty()) {
        return Result<std::string>::failure(ExitCode::GeneralError, "Gagal mendeteksi block device");
    }

    // Baca scheduler aktif dari target block device utama (misal sda)
    std::string primary_dev = dev_res.value().front();
    auto read_res = sysfs::read_text("block/" + primary_dev + "/queue/scheduler");
    if (!read_res.ok()) {
        return Result<std::string>::failure(ExitCode::GeneralError, "Gagal membaca scheduler " + primary_dev);
    }

    auto info = parse_scheduler_line(read_res.value());
    if (info.current.empty()) {
        return Result<std::string>::failure(ExitCode::GeneralError, "Scheduler aktif tidak terdeteksi pada " + primary_dev);
    }

    return Result<std::string>::success(info.current);
}

Result<std::string> set_scheduler(const std::string& name) {
    if (!validate::is_valid_identifier(name)) {
        return Result<std::string>::failure(ExitCode::InvalidArgs, "Nama scheduler tidak valid: " + name);
    }

    auto avail_res = get_available();
    if (!avail_res.ok()) {
        return Result<std::string>::failure(avail_res.code(), avail_res.message());
    }

    if (!validate::is_in_list(name, avail_res.value())) {
        return Result<std::string>::failure(ExitCode::InvalidArgs, "Scheduler tidak didukung storage: " + name);
    }

    auto dev_res = get_block_devices();
    if (!dev_res.ok()) {
        return Result<std::string>::failure(dev_res.code(), dev_res.message());
    }

    for (const auto& dev : dev_res.value()) {
        std::string target_path = "block/" + dev + "/queue/scheduler";
        auto current_res = sysfs::read_text(target_path);
        if (current_res.ok()) {
            auto info = parse_scheduler_line(current_res.value());
            if (!info.current.empty()) {
                state::save_orig_value("io_" + dev, info.current);
            }
        }

        auto write_res = sysfs::write_text(target_path, name);
        if (!write_res.ok()) {
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, dev + ": gagal menulis scheduler");
        }

        if (!verify_with_retry(target_path, name)) {
            auto cur = sysfs::read_text(target_path);
            std::string actual = cur.ok() ? parse_scheduler_line(cur.value()).current : "unknown";
            return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, dev + ": verifikasi scheduler gagal (target: " + name + ", aktual: " + actual + ")");
        }
    }

    state::set_config_value("io_scheduler", name);
    return Result<std::string>::success(name);
}

Result<std::string> reset() {
    auto dev_res = get_block_devices();
    if (!dev_res.ok()) {
        return Result<std::string>::failure(dev_res.code(), dev_res.message());
    }

    for (const auto& dev : dev_res.value()) {
        std::string orig_key = "io_" + dev;
        auto orig_res = state::load_orig_value(orig_key);
        if (orig_res.ok()) {
            std::string target_path = "block/" + dev + "/queue/scheduler";
            auto write_res = sysfs::write_text(target_path, orig_res.value());
            if (!write_res.ok()) {
                return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, dev + ": gagal memulihkan scheduler asli");
            }

            if (!verify_with_retry(target_path, orig_res.value())) {
                return Result<std::string>::failure(ExitCode::WriteOrVerifyFailed, dev + ": verifikasi restore scheduler gagal");
            }
            state::remove_orig_value(orig_key);
        }
    }

    state::remove_config_value("io_scheduler");
    return get_current();
}

} // namespace io
} // namespace aester
