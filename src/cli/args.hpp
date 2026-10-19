#pragma once

#include <string>

namespace aester {
namespace cli {

enum class CommandType {
    Version,
    CpuFreq,
    Governor,
    Io,
    ApplySaved,
    Status,
    Unknown
};

struct ParsedArgs {
    CommandType command = CommandType::Unknown;
    std::string action;
    std::string param;
    bool valid = false;
    std::string error_message;
};

ParsedArgs parse_args(int argc, char* argv[]);

} // namespace cli
} // namespace aester
