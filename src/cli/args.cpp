#include "cli/args.hpp"

namespace aester {
namespace cli {

ParsedArgs parse_args(int argc, char* argv[]) {
    ParsedArgs res;
    if (argc < 2) {
        res.valid = false;
        res.error_message = "argumen tidak lengkap (butuh subperintah)";
        return res;
    }

    std::string cmd = argv[1];
    if (cmd == "version") {
        res.command = CommandType::Version;
        res.valid = true;
        return res;
    }
    if (cmd == "apply-saved") {
        res.command = CommandType::ApplySaved;
        res.valid = true;
        return res;
    }
    if (cmd == "status") {
        res.command = CommandType::Status;
        res.valid = true;
        return res;
    }
    if (cmd == "cpufreq") {
        res.command = CommandType::CpuFreq;
        if (argc < 3) {
            res.valid = false;
            res.error_message = "cpufreq butuh aksi get|set";
            return res;
        }
        res.action = argv[2];
        if (res.action == "set") {
            if (argc < 4) {
                res.valid = false;
                res.error_message = "cpufreq set butuh nilai 40|50|60|70|80|90|off";
                return res;
            }
            res.param = argv[3];
        } else if (res.action != "get") {
            res.valid = false;
            res.error_message = "aksi cpufreq tidak valid: " + res.action;
            return res;
        }
        res.valid = true;
        return res;
    }
    if (cmd == "governor") {
        res.command = CommandType::Governor;
        if (argc < 3) {
            res.valid = false;
            res.error_message = "governor butuh aksi list|get|set";
            return res;
        }
        res.action = argv[2];
        if (res.action == "set") {
            if (argc < 4) {
                res.valid = false;
                res.error_message = "governor set butuh nama governor";
                return res;
            }
            res.param = argv[3];
        } else if (res.action != "list" && res.action != "get") {
            res.valid = false;
            res.error_message = "aksi governor tidak valid: " + res.action;
            return res;
        }
        res.valid = true;
        return res;
    }
    if (cmd == "io") {
        res.command = CommandType::Io;
        if (argc < 3) {
            res.valid = false;
            res.error_message = "io butuh aksi list|get|set";
            return res;
        }
        res.action = argv[2];
        if (res.action == "set") {
            if (argc < 4) {
                res.valid = false;
                res.error_message = "io set butuh nama scheduler";
                return res;
            }
            res.param = argv[3];
        } else if (res.action != "list" && res.action != "get") {
            res.valid = false;
            res.error_message = "aksi io tidak valid: " + res.action;
            return res;
        }
        res.valid = true;
        return res;
    }

    res.valid = false;
    res.error_message = "subperintah tidak dikenal: " + cmd;
    return res;
}

} // namespace cli
} // namespace aester
