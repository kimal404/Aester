#include "features/system/system.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/state.hpp"
#include "features/cpufreq/cpufreq.hpp"
#include "features/governor/governor.hpp"
#include "features/io/io.hpp"

#include <sstream>

namespace aester {
namespace system {

Result<std::string> apply_saved() {
    auto cfg_res = state::load_config();
    if (!cfg_res.ok()) {
        return Result<std::string>::failure(cfg_res.code(), "Gagal membaca konfigurasi tersimpan");
    }

    const auto& config = cfg_res.value();
    std::ostringstream out;
    bool any_applied = false;

    // 1. Terapkan Limit CPU Freq jika ada
    auto it_cpu = config.find("cpufreq");
    if (it_cpu != config.end() && !it_cpu->second.empty()) {
        auto res = cpufreq::set_limit(it_cpu->second);
        if (res.ok()) {
            out << "[cpufreq] diterapkan: " << res.value() << "\n";
            any_applied = true;
        } else {
            log::error("apply-saved: gagal menerapkan cpufreq limit: " + res.message());
        }
    }

    // 2. Terapkan Governor jika ada
    auto it_gov = config.find("governor");
    if (it_gov != config.end() && !it_gov->second.empty()) {
        auto res = governor::set_governor(it_gov->second);
        if (res.ok()) {
            out << "[governor] diterapkan: " << res.value() << "\n";
            any_applied = true;
        } else {
            log::error("apply-saved: gagal menerapkan governor: " + res.message());
        }
    }

    // 3. Terapkan I/O Scheduler jika ada
    auto it_io = config.find("io_scheduler");
    if (it_io != config.end() && !it_io->second.empty()) {
        auto res = io::set_scheduler(it_io->second);
        if (res.ok()) {
            out << "[io_scheduler] diterapkan: " << res.value() << "\n";
            any_applied = true;
        } else {
            log::error("apply-saved: gagal menerapkan io scheduler: " + res.message());
        }
    }

    if (!any_applied) {
        return Result<std::string>::success("Tidak ada konfigurasi aktif untuk diterapkan.\n");
    }

    return Result<std::string>::success(out.str());
}

Result<std::string> get_status_dump() {
    std::ostringstream out;
    out << "Aester Backend: v" << paths::MODULE_VERSION << " (Contract: v" << paths::CONTRACT_VERSION << ")\n";
    out << "-------------------------------------------\n";

    // CPU Freq Status
    auto cpu_res = cpufreq::get_current_limit();
    out << "CPU Freq Limit   : " << (cpu_res.ok() ? cpu_res.value() : "unknown") << "\n";

    // Governor Status
    auto gov_res = governor::get_current();
    out << "CPU Governor     : " << (gov_res.ok() ? gov_res.value() : "unknown") << "\n";

    // I/O Scheduler Status
    auto io_res = io::get_current();
    out << "I/O Scheduler    : " << (io_res.ok() ? io_res.value() : "unknown") << "\n";

    // Block Storage Devices
    auto block_res = io::get_block_devices();
    if (block_res.ok()) {
        out << "Active Devices   : ";
        for (size_t i = 0; i < block_res.value().size(); ++i) {
            out << block_res.value()[i] << (i + 1 < block_res.value().size() ? ", " : "");
        }
        out << "\n";
    }

    return Result<std::string>::success(out.str());
}

} // namespace system
} // namespace aester
