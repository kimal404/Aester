#include "cli/commands.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/result.hpp"
#include "features/cpufreq/cpufreq.hpp"
#include "features/governor/governor.hpp"
#include "features/io/io.hpp"
#include "features/system/system.hpp"

#include <iostream>

namespace aester {
namespace cli {

int execute_command(const ParsedArgs& args) {
    if (!args.valid) {
        log::error(args.error_message);
        return static_cast<int>(ExitCode::InvalidArgs);
    }

    switch (args.command) {
        case CommandType::Version:
            std::cout << paths::MODULE_VERSION << " " << paths::CONTRACT_VERSION << "\n";
            return static_cast<int>(ExitCode::Ok);

        case CommandType::CpuFreq: {
            if (args.action == "get") {
                auto res = cpufreq::get_current_limit();
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            if (args.action == "set") {
                auto res = cpufreq::set_limit(args.param);
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            return static_cast<int>(ExitCode::InvalidArgs);
        }

        case CommandType::Governor: {
            if (args.action == "list") {
                auto res = governor::get_available();
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                for (const auto& gov : res.value()) {
                    std::cout << gov << "\n";
                }
                return static_cast<int>(ExitCode::Ok);
            }
            if (args.action == "get") {
                auto res = governor::get_current();
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            if (args.action == "set") {
                auto res = governor::set_governor(args.param);
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            if (args.action == "reset") {
                auto res = governor::reset();
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            return static_cast<int>(ExitCode::InvalidArgs);
        }

        case CommandType::Io: {
            if (args.action == "list") {
                auto res = io::get_available();
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                for (const auto& s : res.value()) {
                    std::cout << s << "\n";
                }
                return static_cast<int>(ExitCode::Ok);
            }
            if (args.action == "get") {
                auto res = io::get_current();
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            if (args.action == "set") {
                auto res = io::set_scheduler(args.param);
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            if (args.action == "reset") {
                auto res = io::reset();
                if (!res.ok()) {
                    log::error(res.message());
                    return static_cast<int>(res.code());
                }
                std::cout << res.value() << "\n";
                return static_cast<int>(ExitCode::Ok);
            }
            return static_cast<int>(ExitCode::InvalidArgs);
        }

        case CommandType::ApplySaved: {
            auto res = system::apply_saved();
            if (!res.ok()) {
                log::error(res.message());
                return static_cast<int>(res.code());
            }
            std::cout << res.value();
            return static_cast<int>(ExitCode::Ok);
        }

        case CommandType::Status: {
            auto res = system::get_status_dump();
            if (!res.ok()) {
                log::error(res.message());
                return static_cast<int>(res.code());
            }
            std::cout << res.value();
            return static_cast<int>(ExitCode::Ok);
        }

        default:
            log::error("perintah tidak didukung");
            return static_cast<int>(ExitCode::InvalidArgs);
    }
}

} // namespace cli
} // namespace aester
