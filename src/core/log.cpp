#include "core/log.hpp"
#include "core/paths.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace aester {
namespace log {

namespace {

Level current_level = Level::Info;
bool initialized = false;
constexpr off_t MAX_LOG_SIZE = 256 * 1024; // 256 KB

void check_rotate(const std::string& filepath) {
    struct stat st;
    if (::stat(filepath.c_str(), &st) == 0 && st.st_size > MAX_LOG_SIZE) {
        ::truncate(filepath.c_str(), 0);
    }
}

void write_log_file(const char* tag, const std::string& msg) {
    std::string log_file = paths::get_log_path();
    check_rotate(log_file);

    std::ofstream ofs(log_file, std::ios::app);
    if (ofs.is_open()) {
        ofs << "[" << tag << "] " << msg << "\n";
    }
}

} // namespace

void init_log() {
    if (initialized) return;
    initialized = true;

    const char* env = std::getenv("AESTER_LOG");
    if (env) {
        if (std::strcmp(env, "debug") == 0) {
            current_level = Level::Debug;
        } else if (std::strcmp(env, "error") == 0) {
            current_level = Level::Error;
        } else {
            current_level = Level::Info;
        }
    }
}

void error(const std::string& message) {
    init_log();
    std::cerr << "error: " << message << "\n";
    write_log_file("ERROR", message);
}

void info(const std::string& message) {
    init_log();
    if (current_level >= Level::Info) {
        std::cerr << "info: " << message << "\n";
    }
    write_log_file("INFO", message);
}

void debug(const std::string& message) {
    init_log();
    if (current_level >= Level::Debug) {
        std::cerr << "debug: " << message << "\n";
    }
    write_log_file("DEBUG", message);
}

} // namespace log
} // namespace aester
