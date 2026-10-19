#include "cli/commands.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "core/result.hpp"

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

        case CommandType::CpuFreq:
        case CommandType::Governor:
        case CommandType::Io:
        case CommandType::ApplySaved:
        case CommandType::Status:
            log::error("belum diimplementasi (Batch A)");
            return static_cast<int>(ExitCode::NotSupported);

        default:
            log::error("perintah tidak didukung");
            return static_cast<int>(ExitCode::InvalidArgs);
    }
}

} // namespace cli
} // namespace aester
