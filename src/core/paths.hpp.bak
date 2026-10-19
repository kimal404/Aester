#pragma once

#include <cstdlib>
#include <string>

namespace aester {
namespace paths {

constexpr const char* MODULE_VERSION = "1.0.0";
constexpr int CONTRACT_VERSION = 1;

inline std::string get_sysfs_root() {
    const char* env = std::getenv("AESTER_SYSFS_ROOT");
    if (env && env[0] != '\0') {
        return std::string(env);
    }
    return "/sys";
}

inline std::string get_state_dir() {
    const char* env = std::getenv("AESTER_STATE_DIR");
    if (env && env[0] != '\0') {
        return std::string(env);
    }
    return "/data/adb/aester";
}

inline std::string get_config_path() {
    return get_state_dir() + "/config";
}

inline std::string get_orig_dir() {
    return get_state_dir() + "/orig";
}

inline std::string get_log_path() {
    return get_state_dir() + "/aester.log";
}

} // namespace paths
} // namespace aester
