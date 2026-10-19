#include "core/sysfs.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace aester {
namespace sysfs {

std::string resolve_path(const std::string& sysfs_path) {
    std::string root = paths::get_sysfs_root();
    if (sysfs_path.empty()) return root;

    if (sysfs_path.rfind("/sys/", 0) == 0) {
        return root + sysfs_path.substr(4);
    }
    if (sysfs_path == "/sys") {
        return root;
    }
    if (sysfs_path[0] == '/') {
        return root + sysfs_path;
    }
    return root + "/" + sysfs_path;
}

Result<std::string> read_text(const std::string& sysfs_path) {
    std::string full_path = resolve_path(sysfs_path);
    std::ifstream ifs(full_path);
    if (!ifs.is_open()) {
        std::string err_msg = full_path + ": " + std::strerror(errno);
        log::debug("read failed: " + err_msg);
        return Result<std::string>::failure(ExitCode::GeneralError, err_msg);
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string content = buffer.str();

    while (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
        content.pop_back();
    }

    log::debug("read [" + full_path + "] -> " + content);
    return Result<std::string>::success(content);
}

Result<void> write_text(const std::string& sysfs_path, const std::string& value) {
    std::string full_path = resolve_path(sysfs_path);
    const char* dry_run = std::getenv("AESTER_DRYRUN");
    bool is_dry_run = (dry_run && dry_run[0] == '1');

    log::debug(std::string(is_dry_run ? "dry-run write [" : "write [") +
               full_path + "] <- " + value);

    if (is_dry_run) {
        return Result<void>::success();
    }

    std::ofstream ofs(full_path);
    if (!ofs.is_open()) {
        std::string err_msg = full_path + ": " + std::strerror(errno);
        log::error("write failed: " + err_msg);
        return Result<void>::failure(ExitCode::WriteOrVerifyFailed, err_msg);
    }

    ofs << value;
    if (!ofs.good()) {
        std::string err_msg = full_path + ": write flush failed - " + std::strerror(errno);
        log::error(err_msg);
        return Result<void>::failure(ExitCode::WriteOrVerifyFailed, err_msg);
    }

    return Result<void>::success();
}

Result<std::vector<std::string>> list_dir(const std::string& sysfs_path) {
    std::string full_path = resolve_path(sysfs_path);
    DIR* dir = ::opendir(full_path.c_str());
    if (!dir) {
        std::string err_msg = full_path + ": " + std::strerror(errno);
        return Result<std::vector<std::string>>::failure(ExitCode::GeneralError, err_msg);
    }

    std::vector<std::string> entries;
    struct dirent* entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        entries.emplace_back(entry->d_name);
    }
    ::closedir(dir);
    return Result<std::vector<std::string>>::success(entries);
}

bool exists(const std::string& sysfs_path) {
    std::string full_path = resolve_path(sysfs_path);
    struct stat st;
    return (::stat(full_path.c_str(), &st) == 0);
}

} // namespace sysfs
} // namespace aester
