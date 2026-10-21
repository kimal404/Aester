#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>

#include "features/cpufreq/cpufreq.hpp"
#include "features/governor/governor.hpp"
#include "features/io/io.hpp"

#define SOCKET_NAME "aester_daemon"

std::string handle_command(const std::string& cmd) {
    if (cmd == "PING") {
        return "PONG\n";
    }
    if (cmd == "CPUFREQ:GET") {
        auto res = aester::cpufreq::get_current_limit();
        return res.ok() ? res.value() + "\n" : "ERR " + res.message() + "\n";
    }
    if (cmd.rfind("CPUFREQ:SET ", 0) == 0) {
        std::string val = cmd.substr(12);
        auto res = aester::cpufreq::set_limit(val);
        return res.ok() ? "OK " + res.value() + "\n" : "ERR " + res.message() + "\n";
    }
    if (cmd == "GOV:LIST") {
        auto res = aester::governor::get_available();
        if (!res.ok()) return "ERR " + res.message() + "\n";
        std::string out;
        for (size_t i = 0; i < res.value().size(); ++i) {
            out += res.value()[i] + (i + 1 < res.value().size() ? "," : "");
        }
        return out + "\n";
    }
    if (cmd == "GOV:GET") {
        auto res = aester::governor::get_current();
        return res.ok() ? res.value() + "\n" : "ERR " + res.message() + "\n";
    }
    if (cmd.rfind("GOV:SET ", 0) == 0) {
        std::string val = cmd.substr(8);
        auto res = aester::governor::set_governor(val);
        return res.ok() ? "OK " + res.value() + "\n" : "ERR " + res.message() + "\n";
    }
    if (cmd == "IO:LIST") {
        auto res = aester::io::get_available();
        if (!res.ok()) return "ERR " + res.message() + "\n";
        std::string out;
        for (size_t i = 0; i < res.value().size(); ++i) {
            out += res.value()[i] + (i + 1 < res.value().size() ? "," : "");
        }
        return out + "\n";
    }
    if (cmd == "IO:GET") {
        auto res = aester::io::get_current();
        return res.ok() ? res.value() + "\n" : "ERR " + res.message() + "\n";
    }
    if (cmd.rfind("IO:SET ", 0) == 0) {
        std::string val = cmd.substr(7);
        auto res = aester::io::set_scheduler(val);
        return res.ok() ? "OK " + res.value() + "\n" : "ERR " + res.message() + "\n";
    }
    return "ERR Unknown command\n";
}

int main() {
    // Melepaskan proses ke background sebagai system daemon mandiri (PID 1 init)
    if (daemon(1, 0) < 0) {
        return 1;
    }

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

        char buf[512] = {0};
        ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            std::string cmd(buf);
            while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) cmd.pop_back();
            std::string resp = handle_command(cmd);
            write(client_fd, resp.c_str(), resp.length());
        }
        close(client_fd);
    }
    close(server_fd);
    return 0;
}
