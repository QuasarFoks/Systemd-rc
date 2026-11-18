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

const char* green = "\033[32m";
const char* red   = "\033[31m";
const char* reset = "\033[0m";

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

int runCommandF(const std::string& name, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(name.c_str()));
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execvp(name.c_str(), argv.data());
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// ---------------------------------------------------------------------------
bool isRoot() { return geteuid() == 0; }

// ---------------------------------------------------------------------------
bool isServiceEnabled(const std::string& service) {
    if (service.empty()) return false;
    if (service.find('/')  != std::string::npos) return false;
    if (service.find('\\') != std::string::npos) return false;
    if (service == "." || service == "..")       return false;

    std::string path = "/var/service/" + service;
    struct stat st;
    if (lstat(path.c_str(), &st) != 0) return false;
    return S_ISLNK(st.st_mode);
}

// ---------------------------------------------------------------------------
void Help() {
    std::cout << "\n";
    std::cout << "FLAGS                   Experimental flag \n";
    std::cout << "============Service management============\n";
    std::cout << "enable       {SERVICE}       --now        \n";
    std::cout << "disable      {SERVICE}                    \n";
    std::cout << "status       {SERVICE}                    \n";
    std::cout << "restart      {SERVICE}                    \n";
    std::cout << "reload       {SERVICE}                    \n";
    std::cout << "stop         {SERVICE}                    \n";
    std::cout << "start        {SERVICE}                    \n";
    std::cout << "is-enabled   {SERVICE}                    \n";
    std::cout << "daemon-reload                             \n";
    std::cout << "list-unit                                 \n";
    std::cout << "list-unit-files              --user       \n";
    std::cout << "=============Power management=============\n";
    std::cout << "halt                                      \n";
    std::cout << "poweroff                                  \n";
    std::cout << "reboot                                    \n";
    std::cout << "suspend                                   \n";
    std::cout << "hibernate                                 \n";
    std::cout << "==========================================\n";
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: systemctl [OPTIONS...] COMMAND [SERVICE...]\n";
        return 2;
    }

    bool nowFlag = false;
    int pos;

    // Находим позицию первой НЕ-флаговой команды
    for (pos = 1; pos < argc; pos++) {
        std::string arg = argv[pos];
        if (arg == "--now") {
            nowFlag = true;
        } else if (arg == "-v" || arg == "--verbose") {
            continue;
        } else if (!arg.empty() && arg[0] == '-') {
            continue; // unknown flag — skip
        } else {
            break;
        }
    }

    if (pos >= argc) {
        std::cerr << "error: no command specified\n";
        return 2;
    }

    std::string command = argv[pos];
    std::vector<std::string> services;
    for (int i = pos + 1; i < argc; i++) services.push_back(argv[i]);

    // =====================================================================
    if (command == "enable") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& service : services) {
            if (!isRoot()) {
                if (nowFlag) {
                    int rc = runCommandF("pkexec", {"/usr/bin/sv", "enable", service});
                    if (rc == 0) {
                        std::cout << "Created symlink /etc/service/" << service
                                  << " --> /etc/sv/" << service << "\n";
                        runCommandF("pkexec", {"/usr/bin/sv", "up", service});
                    } else {
                        std::cout << "Error enabling " << service << "\n";
                        return 1;
                    }
                } else {
                    int rc = runCommandF("pkexec", {"/usr/bin/sv", "enable", service});
                    if (rc == 0) {
                        std::cout << "Created symlink /etc/service/" << service
                                  << " --> /etc/sv/" << service << "\n";
                    } else {
                        std::cout << "Error enabling " << service << "\n";
                        return 1;
                    }
                }
            } else {
                int rc = runCommandF("/usr/bin/sv", {"enable", service});
                if (rc == 0) {
                    std::cout << "Created symlink /etc/service/" << service
                              << " --> /etc/sv/" << service << "\n";
                } else {
                    std::cout << "Error enabling " << service << "\n";
                    return 1;
                }
            }
        }
    }
    // =====================================================================
    else if (command == "disable") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& service : services) {
            if (!isRoot()) {
                int rc = runCommandF("pkexec", {"sv", "disable", service});
                if (rc == 0) {
                    std::cout << "Removed /etc/service/" << service << "\n";
                } else {
                    std::cout << "Error disabling " << service << "\n";
                    return 1;
                }
            } else {
                int rc = runCommandF("sv", {"disable", service});
                if (rc == 0) {
                    std::cout << "Removed /etc/service/" << service << "\n";
                } else {
                    std::cout << "Error disabling " << service << "\n";
                    return 1;
                }
            }
        }
    }
    // =====================================================================
    else if (command == "status") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& service : services) {
            if (!isRoot()) {
                runCommand("pkexec", {"/usr/bin/sv", "status", service});
            } else {
                runCommand("/usr/bin/sv", {"status", service});
            }
        }
    }
    // =====================================================================
    else if (command == "start") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& service : services) {
            if (!isRoot()) {
                runCommand("pkexec", {"/usr/bin/sv", "up", service});
            } else {
                runCommand("/usr/bin/sv", {"up", service});
            }
        }
    }
    // =====================================================================
    else if (command == "stop") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& service : services) {
            if (!isRoot()) {
                runCommand("pkexec", {"/usr/bin/sv", "down", service});
            } else {
                runCommand("/usr/bin/sv", {"down", service});
            }
        }
    }
    // =====================================================================
    else if (command == "reload") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& service : services) {
            if (!isRoot()) {
                runCommandF("pkexec", {"/usr/bin/sv", "hup", service});
            } else {
                runCommandF("/usr/bin/sv", {"hup", service});
            }
        }
    }
    // =====================================================================
    else if (command == "restart") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& service : services) {
            if (!isRoot()) {
                runCommandF("pkexec", {"/usr/bin/sv", "restart", service});
            } else {
                runCommandF("/usr/bin/sv", {"restart", service});
            }
        }
    }
    // =====================================================================
    else if (command == "is-enabled") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        bool allEnabled = true;
        for (const auto& service : services) {
            if (isServiceEnabled(service)) {
                std::cout << service << " enabled\n";
            } else {
                std::cout << service << " disabled\n";
                allEnabled = false;
            }
        }
        if (!allEnabled) return 1;
    }
    // =====================================================================
    else if (command == "list-units") {
        runCommand("sv", {"-l"});
    }
    // =====================================================================
    else if (command == "halt")     { runCommand("halt", {}); }
    else if (command == "poweroff") { runCommand("poweroff", {}); }
    else if (command == "reboot")   { runCommand("reboot", {}); }
    else if (command == "help")     { Help(); }
    // =====================================================================
    else {
        std::cerr << "Unknown command: " << command << "\n";
        return 2;
    }

    return 0;
}
