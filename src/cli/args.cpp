#include "cli/args.hpp"

namespace aester {
namespace cli {

ParsedArgs parse_args(int argc, char* argv[]) {
    ParsedArgs parsed;
    parsed.valid = false;

    if (argc < 2) {
        parsed.error_message = "argumen tidak cukup. Gunakan: aesterctl <command> [action] [param]";
        return parsed;
    }

    std::string cmd_str = argv[1];

    if (cmd_str == "version") {
        parsed.command = CommandType::Version;
        parsed.valid = true;
        return parsed;
    }

    if (cmd_str == "cpufreq") {
        parsed.command = CommandType::CpuFreq;
        if (argc < 3) {
            parsed.error_message = "cpufreq memerlukan aksi (get, set)";
            return parsed;
        }
        std::string action = argv[2];
        parsed.action = action;
        if (action == "get") {
            parsed.valid = true;
            return parsed;
        }
        if (action == "set") {
            if (argc < 4) {
                parsed.error_message = "cpufreq set memerlukan nilai (40..90 atau off)";
                return parsed;
            }
            parsed.param = argv[3];
            parsed.valid = true;
            return parsed;
        }
        parsed.error_message = "aksi cpufreq tidak valid: " + action;
        return parsed;
    }

    if (cmd_str == "governor") {
        parsed.command = CommandType::Governor;
        if (argc < 3) {
            parsed.error_message = "governor memerlukan aksi (list, get, set, reset)";
            return parsed;
        }
        std::string action = argv[2];
        parsed.action = action;
        if (action == "list" || action == "get" || action == "reset") {
            parsed.valid = true;
            return parsed;
        }
        if (action == "set") {
            if (argc < 4) {
                parsed.error_message = "governor set memerlukan nama governor";
                return parsed;
            }
            parsed.param = argv[3];
            parsed.valid = true;
            return parsed;
        }
        parsed.error_message = "aksi governor tidak valid: " + action;
        return parsed;
    }

    if (cmd_str == "io") {
        parsed.command = CommandType::Io;
        if (argc < 3) {
            parsed.error_message = "io memerlukan aksi (list, get, set, reset)";
            return parsed;
        }
        parsed.action = argv[2];
        if (parsed.action == "set" && argc >= 4) {
            parsed.param = argv[3];
        }
        parsed.valid = true;
        return parsed;
    }

    if (cmd_str == "apply-saved") {
        parsed.command = CommandType::ApplySaved;
        parsed.valid = true;
        return parsed;
    }

    if (cmd_str == "status") {
        parsed.command = CommandType::Status;
        parsed.valid = true;
        return parsed;
    }

    parsed.error_message = "perintah tidak dikenal: " + cmd_str;
    return parsed;
}

} // namespace cli
} // namespace aester
