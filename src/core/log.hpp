#pragma once

#include <string>

namespace aester {
namespace log {

enum class Level {
    Error,
    Info,
    Debug
};

void init_log();
void error(const std::string& message);
void info(const std::string& message);
void debug(const std::string& message);

} // namespace log
} // namespace aester
