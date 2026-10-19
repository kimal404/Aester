#include "core/state.hpp"
#include "core/paths.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace aester {
namespace state {

namespace {

void make_dir_p(const std::string& dir) {
    char tmp[512];
    std::snprintf(tmp, sizeof(tmp), "%s", dir.c_str());
    size_t len = std::strlen(tmp);
    for (size_t i = 1; i < len; ++i) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            ::mkdir(tmp, 0755);
            tmp[i] = '/';
        }
    }
    ::mkdir(tmp, 0755);
}

} // namespace

Result<void> init_dirs() {
    make_dir_p(paths::get_state_dir());
    make_dir_p(paths::get_orig_dir());
    return Result<void>::success();
}

Result<std::map<std::string, std::string>> load_config() {
    init_dirs();
    std::string cfg_path = paths::get_config_path();
    std::ifstream ifs(cfg_path);
    if (!ifs.is_open()) {
        return Result<std::map<std::string, std::string>>::success({});
    }

    std::map<std::string, std::string> map;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string k = line.substr(0, eq);
            std::string v = line.substr(eq + 1);
            while (!v.empty() && (v.back() == '\r' || v.back() == '\n')) v.pop_back();
            map[k] = v;
        }
    }
    return Result<std::map<std::string, std::string>>::success(map);
}

Result<std::string> get_config_value(const std::string& key) {
    auto res = load_config();
    if (!res.ok()) return Result<std::string>::failure(res.code(), res.message());

    auto it = res.value().find(key);
    if (it != res.value().end()) {
        return Result<std::string>::success(it->second);
    }
    return Result<std::string>::failure(ExitCode::GeneralError, "Key not found: " + key);
}

Result<void> set_config_value(const std::string& key, const std::string& value) {
    auto cfg_res = load_config();
    std::map<std::string, std::string> map = cfg_res.ok() ? cfg_res.value() : std::map<std::string, std::string>{};
    map[key] = value;

    std::string cfg_path = paths::get_config_path();
    std::string tmp_path = cfg_path + ".tmp." + std::to_string(::getpid());

    std::ofstream ofs(tmp_path);
    if (!ofs.is_open()) {
        return Result<void>::failure(ExitCode::GeneralError, "Cannot open tmp config: " + std::string(std::strerror(errno)));
    }

    for (const auto& pair : map) {
        ofs << pair.first << "=" << pair.second << "\n";
    }
    ofs.flush();
    ofs.close();

    if (::rename(tmp_path.c_str(), cfg_path.c_str()) != 0) {
        ::unlink(tmp_path.c_str());
        return Result<void>::failure(ExitCode::GeneralError, "Atomic rename config failed: " + std::string(std::strerror(errno)));
    }

    return Result<void>::success();
}

Result<void> save_orig_value(const std::string& name, const std::string& value) {
    init_dirs();
    std::string path = paths::get_orig_dir() + "/" + name;
    if (has_orig_value(name)) {
        return Result<void>::success();
    }

    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        return Result<void>::failure(ExitCode::GeneralError, path + ": " + std::strerror(errno));
    }
    ofs << value << "\n";
    return Result<void>::success();
}

Result<std::string> load_orig_value(const std::string& name) {
    std::string path = paths::get_orig_dir() + "/" + name;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return Result<std::string>::failure(ExitCode::GeneralError, "Original value not found for: " + name);
    }
    std::string val;
    std::getline(ifs, val);
    while (!val.empty() && (val.back() == '\r' || val.back() == '\n')) val.pop_back();
    return Result<std::string>::success(val);
}

bool has_orig_value(const std::string& name) {
    std::string path = paths::get_orig_dir() + "/" + name;
    struct stat st;
    return (::stat(path.c_str(), &st) == 0);
}

Result<void> remove_orig_value(const std::string& name) {
    std::string path = paths::get_orig_dir() + "/" + name;
    ::unlink(path.c_str());
    return Result<void>::success();
}

} // namespace state
} // namespace aester
