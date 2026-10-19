#include "cli/commands.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/result.hpp"
#include "features/cpufreq/cpufreq.hpp"

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

        case CommandType::Governor:
        case CommandType::Io:
        case CommandType::ApplySaved:
        case CommandType::Status:
            log::error("belum diimplementasi");
            return static_cast<int>(ExitCode::NotSupported);

        default:
            log::error("perintah tidak didukung");
            return static_cast<int>(ExitCode::InvalidArgs);
    }
}

} // namespace cli
} // namespace aester
