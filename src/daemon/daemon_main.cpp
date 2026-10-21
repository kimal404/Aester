#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#define SOCKET_NAME "aester_daemon"
#define CPU_BASE "/sys/devices/system/cpu/cpufreq"

static std::string read_sysfs(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string line;
    std::getline(file, line);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
    return line;
}

static bool write_sysfs(const std::string& path, const std::string& val) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << val << std::endl;
    return file.good();
}

// Menulis sysfs dengan penanganan kuncian chmod dan thermal bypass mi_thermald
static bool write_sysfs_with_bypass(const std::string& path, long target_freq) {
    std::string val_str = std::to_string(target_freq);

    // 1. Buka proteksi berkas ke mode tulis
    chmod(path.c_str(), 0644);
    write_sysfs(path, val_str);

    // 2. Cek apakah kernel menerima langsung
    std::string rb = read_sysfs(path);
    if (!rb.empty() && std::stol("0" + rb) == target_freq) {
        chmod(path.c_str(), 0444); // Kunci read-only
        return true;
    }

    // 3. Jika nilai terpotong oleh QoS daemon vendor, lakukan thermal bypass window
    system("killall -STOP mi_thermald 2>/dev/null");
    chmod(path.c_str(), 0644);
    write_sysfs(path, val_str);
    rb = read_sysfs(path);
    system("killall -CONT mi_thermald 2>/dev/null");

    if (!rb.empty() && std::stol("0" + rb) == target_freq) {
        chmod(path.c_str(), 0444); // Kunci agar vendor tidak menimpa balik
        return true;
    }

    // Kembalikan ke izin standar jika gagal
    chmod(path.c_str(), 0644);
    return false;
}

struct PolicySnapshot {
    std::string id;
    std::string label;
    std::string cores;
    std::vector<long> freqs;
    long min_freq = 0;
    long max_freq = 0;
    long cur_freq = 0;
    std::string governor;
    std::vector<std::string> governors;
};

static std::vector<PolicySnapshot> detect_cpu_topology() {
    std::vector<PolicySnapshot> list;
    DIR* dir = opendir(CPU_BASE);
    if (!dir) return list;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "policy", 6) == 0) {
            std::string p_name = entry->d_name;
            std::string p_path = std::string(CPU_BASE) + "/" + p_name;

            PolicySnapshot snap;
            snap.id = p_name;
            snap.cores = read_sysfs(p_path + "/related_cpus");
            if (snap.cores.empty()) {
                snap.cores = read_sysfs(p_path + "/affected_cpus");
            }

            std::string f_str = read_sysfs(p_path + "/scaling_available_frequencies");
            std::stringstream ss(f_str);
            long f;
            while (ss >> f) {
                snap.freqs.push_back(f);
            }
            std::sort(snap.freqs.begin(), snap.freqs.end());

            snap.min_freq = std::stol("0" + read_sysfs(p_path + "/scaling_min_freq"));
            snap.max_freq = std::stol("0" + read_sysfs(p_path + "/scaling_max_freq"));

            std::string cur = read_sysfs(p_path + "/scaling_cur_freq");
            if (cur.empty()) cur = read_sysfs(p_path + "/cpuinfo_cur_freq");
            snap.cur_freq = std::stol("0" + cur);

            snap.governor = read_sysfs(p_path + "/scaling_governor");
            std::string g_str = read_sysfs(p_path + "/scaling_available_governors");
            std::stringstream g_ss(g_str);
            std::string g;
            while (g_ss >> g) {
                snap.governors.push_back(g);
            }

            list.push_back(snap);
        }
    }
    closedir(dir);

    std::sort(list.begin(), list.end(), [](const PolicySnapshot& a, const PolicySnapshot& b) {
        long max_a = a.freqs.empty() ? 0 : a.freqs.back();
        long max_b = b.freqs.empty() ? 0 : b.freqs.back();
        return max_a < max_b;
    });

    for (size_t i = 0; i < list.size(); ++i) {
        if (list.size() == 1) {
            list[i].label = "Single Cluster (Cores " + list[i].cores + ")";
        } else if (list.size() == 2) {
            list[i].label = (i == 0) ? "Little Core (Cores " + list[i].cores + ")" 
                                     : "Big Core (Cores " + list[i].cores + ")";
        } else if (list.size() == 3) {
            if (i == 0) list[i].label = "Little Core (Cores " + list[i].cores + ")";
            else if (i == 1) list[i].label = "Big Core (Cores " + list[i].cores + ")";
            else list[i].label = "Prime Core (Cores " + list[i].cores + ")";
        } else {
            list[i].label = "Cluster " + std::to_string(i) + " (Cores " + list[i].cores + ")";
        }
    }

    return list;
}

static std::string handle_ipc_command(const std::string& cmd) {
    if (cmd == "PING") return "PONG\n";

    if (cmd == "CPU:TOPOLOGY") {
        auto list = detect_cpu_topology();
        std::string json = "[";
        for (size_t i = 0; i < list.size(); ++i) {
            const auto& p = list[i];
            json += "{\"id\":\"" + p.id + "\",\"label\":\"" + p.label + "\",\"coreIds\":\"" + p.cores + "\",";
            json += "\"minFreq\":" + std::to_string(p.min_freq) + ",\"maxFreq\":" + std::to_string(p.max_freq) + ",";
            json += "\"realtimeFreq\":" + std::to_string(p.cur_freq) + ",\"governor\":\"" + p.governor + "\",";
            json += "\"availableFreqs\":[";
            for (size_t j = 0; j < p.freqs.size(); ++j) {
                json += std::to_string(p.freqs[j]) + (j + 1 < p.freqs.size() ? "," : "");
            }
            json += "],\"availableGovernors\":[";
            for (size_t j = 0; j < p.governors.size(); ++j) {
                json += "\"" + p.governors[j] + "\"" + (j + 1 < p.governors.size() ? "," : "");
            }
            json += "]}";
            if (i + 1 < list.size()) json += ",";
        }
        json += "]\n";
        return json;
    }

    if (cmd == "CPU:REALTIME") {
        auto list = detect_cpu_topology();
        std::string out;
        for (size_t i = 0; i < list.size(); ++i) {
            out += list[i].id + ":" + std::to_string(list[i].cur_freq) + (i + 1 < list.size() ? "," : "");
        }
        return out + "\n";
    }

    // Urutan tulis aman + thermal bypass + sysfs lock
    if (cmd.rfind("CPU:FREQ:SET ", 0) == 0) {
        std::stringstream ss(cmd.substr(13));
        std::string policy, type;
        long target_freq;
        if (!(ss >> policy >> type >> target_freq)) return "ERR Format tidak valid\n";

        std::string min_p = std::string(CPU_BASE) + "/" + policy + "/scaling_min_freq";
        std::string max_p = std::string(CPU_BASE) + "/" + policy + "/scaling_max_freq";

        chmod(min_p.c_str(), 0644);
        chmod(max_p.c_str(), 0644);

        long cur_min = std::stol("0" + read_sysfs(min_p));
        long cur_max = std::stol("0" + read_sysfs(max_p));

        if (type == "max") {
            // Jika max baru lebih kecil dari min saat ini, turunkan min terlebih dahulu
            if (target_freq < cur_min) {
                write_sysfs_with_bypass(min_p, target_freq);
            }
            if (!write_sysfs_with_bypass(max_p, target_freq)) {
                write_sysfs_with_bypass(min_p, cur_min); // Rollback
                return "ERR Verifikasi kernel gagal\n";
            }
            chmod(min_p.c_str(), 0444);
            chmod(max_p.c_str(), 0444);
            return "OK\n";
        } else if (type == "min") {
            // Jika min baru lebih besar dari max saat ini, naikkan max terlebih dahulu
            if (target_freq > cur_max) {
                write_sysfs_with_bypass(max_p, target_freq);
            }
            if (!write_sysfs_with_bypass(min_p, target_freq)) {
                write_sysfs_with_bypass(max_p, cur_max); // Rollback
                return "ERR Verifikasi kernel gagal\n";
            }
            chmod(min_p.c_str(), 0444);
            chmod(max_p.c_str(), 0444);
            return "OK\n";
        }
        return "ERR Tipe frekuensi salah\n";
    }

    if (cmd.rfind("CPU:GOV:SET ", 0) == 0) {
        std::stringstream ss(cmd.substr(12));
        std::string policy, gov;
        if (!(ss >> policy >> gov)) return "ERR Format tidak valid\n";
        std::string path = std::string(CPU_BASE) + "/" + policy + "/scaling_governor";
        chmod(path.c_str(), 0644);
        if (write_sysfs(path, gov)) {
            chmod(path.c_str(), 0444);
            return "OK\n";
        }
        return "ERR Gagal menerapkan governor\n";
    }

    if (cmd.rfind("CPU:GOV:TUNABLES ", 0) == 0) {
        std::string policy = cmd.substr(17);
        std::string gov = read_sysfs(std::string(CPU_BASE) + "/" + policy + "/scaling_governor");
        std::string tun_dir = std::string(CPU_BASE) + "/" + policy + "/" + gov;

        DIR* dir = opendir(tun_dir.c_str());
        if (!dir) {
            tun_dir = std::string(CPU_BASE) + "/" + gov;
            dir = opendir(tun_dir.c_str());
        }
        if (!dir) return "[]\n";

        std::string json = "[";
        struct dirent* ent;
        bool first = true;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_name[0] == '.') continue;
            std::string fpath = tun_dir + "/" + ent->d_name;
            struct stat st;
            if (stat(fpath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
                std::string val = read_sysfs(fpath);
                bool ro = (access(fpath.c_str(), W_OK) != 0);
                if (!first) json += ",";
                json += "{\"name\":\"" + std::string(ent->d_name) + "\",\"value\":\"" + val + "\",\"readonly\":" + (ro ? "true" : "false") + "}";
                first = false;
            }
        }
        closedir(dir);
        json += "]\n";
        return json;
    }

    if (cmd.rfind("CPU:GOV:TUNABLE:SET ", 0) == 0) {
        std::stringstream ss(cmd.substr(20));
        std::string policy, key, val;
        if (!(ss >> policy >> key >> val)) return "ERR Format tidak valid\n";

        std::string gov = read_sysfs(std::string(CPU_BASE) + "/" + policy + "/scaling_governor");
        std::string fpath = std::string(CPU_BASE) + "/" + policy + "/" + gov + "/" + key;
        if (access(fpath.c_str(), W_OK) != 0) {
            fpath = std::string(CPU_BASE) + "/" + gov + "/" + key;
        }

        chmod(fpath.c_str(), 0644);
        if (write_sysfs(fpath, val)) return "OK\n";
        return "ERR Gagal menulis tunable sysfs\n";
    }

    return "ERR Unknown command\n";
}

int main() {
    if (daemon(1, 0) < 0) return 1;

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) return 1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path + 1, SOCKET_NAME, sizeof(addr.sun_path) - 2);

    socklen_t addr_len = sizeof(sa_family_t) + 1 + strlen(SOCKET_NAME);
    if (bind(server_fd, (struct sockaddr*)&addr, addr_len) < 0) {
        close(server_fd);
        return 1;
    }

    listen(server_fd, 8);
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) continue;

        char buf[1024] = {0};
        ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            std::string cmd(buf);
            while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
                cmd.pop_back();
            }
            std::string resp = handle_ipc_command(cmd);
            write(client_fd, resp.c_str(), resp.length());
        }
        close(client_fd);
    }
    close(server_fd);
    return 0;
}
