// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// ---------------------------------------------------------------------------
int runCommand(const std::string& name, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(name.c_str()));
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(name.c_str(), argv.data());
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// ---------------------------------------------------------------------------
bool isServiceEnabled(const std::string& service) {
    if (service.empty()) return false;
    if (service.find('/')  != std::string::npos) return false;
    if (service.find('\\') != std::string::npos) return false;
    if (service == "." || service == "..")       return false;

    std::string path = "/etc/runlevels/default/" + service;
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) return false;
    return S_ISLNK(st.st_mode);
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: systemctl <command> [service]\n";
        return 2;
    }

    std::string command = argv[1];
    std::string service;
    if (argc > 2) service = argv[2];

    auto requireService = [&]() -> bool {
        if (service.empty()) {
            std::cerr << "error: service name required\n";
            return false;
        }
        return true;
    };

    // =====================================================================
    if (command == "enable") {
        if (!requireService()) return 2;
        runCommand("dinitctl", {"enable", service});
    }
    // =====================================================================
    else if (command == "disable") {
        if (!requireService()) return 2;
        runCommand("dinitctl", {"disable", service});
    }
    // =====================================================================
    else if (command == "status") {
        if (!requireService()) return 2;
        runCommand("dinitctl", {"status", service});
    }
    // =====================================================================
    else if (command == "start") {
        if (!requireService()) return 2;
        runCommand("dinitctl", {"start", service});
    }
    // =====================================================================
    else if (command == "stop") {
        if (!requireService()) return 2;
        runCommand("dinitctl", {"stop", service});
    }
    // =====================================================================
    else if (command == "reload") {
        if (!requireService()) return 2;
        runCommand("dinitctl", {"reload", service});
    }
    // =====================================================================
    else if (command == "restart") {
        if (!requireService()) return 2;
        runCommand("dinitctl", {"restart", service});
    }
    // =====================================================================
    else if (command == "list-units") {
        runCommand("dinitctl", {"list"});
    }
    // =====================================================================
    else if (command == "halt")     { runCommand("halt", {}); }
    else if (command == "poweroff") { runCommand("poweroff", {}); }
    else if (command == "reboot")   { runCommand("reboot", {}); }
    // =====================================================================
    else if (command == "is-enabled") {
        if (!requireService()) return 2;
        if (isServiceEnabled(service)) {
            std::cout << "enabled\n";
            return 0;
        } else {
            std::cout << "disabled\n";
            return 1;
        }
    }
    // =====================================================================
    else if (command == "list-unit-files") {
        runCommand("dinitctl", {"show"});
    }
    // =====================================================================
    else if (command == "suspend") {
        runCommand("loginctl", {"suspend"});
    }
    // =====================================================================
    else if (command == "hibernate") {
        runCommand("loginctl", {"hibernate"});
    }
    // =====================================================================
    else {
        std::cerr << "Unknown command: " << command << "\n";
        return 2;
    }

    return 0;
}
