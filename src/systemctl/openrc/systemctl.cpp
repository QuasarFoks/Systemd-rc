#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <ctime>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
//
const char* green = "\033[32m";
const char* red   = "\033[31m";
const char* reset = "\033[0m";

// ---------------------------------------------------------------------------
// Запуск команды с наследованием stdout/stderr
// ---------------------------------------------------------------------------
int runCommand(const std::string& name, const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(name.c_str()));
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        execvp(name.c_str(), argv.data());
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

// ---------------------------------------------------------------------------
// Запуск команды с подавлением stdout/stderr
// ---------------------------------------------------------------------------
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
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

// ---------------------------------------------------------------------------
bool isRoot() {
    return geteuid() == 0;
}

// ---------------------------------------------------------------------------
// Проверка, включена ли служба в автозагрузку (симлинк в runlevels/default)
// ---------------------------------------------------------------------------
bool isServiceEnabled(const std::string& service, bool userMode) {
    if (service.empty()) return false;
    if (service.find('/')  != std::string::npos) return false;
    if (service.find('\\') != std::string::npos) return false;
    if (service == "." || service == "..")       return false;

    std::string path;
    if (userMode) {
        const char* home = getenv("HOME");
        path = std::string(home ? home : "") +
               "/.local/share/openrc/runlevels/default/" + service;
    } else {
        path = "/etc/runlevels/default/" + service;
    }

    struct stat st;
    if (lstat(path.c_str(), &st) != 0) return false;
    return S_ISLNK(st.st_mode);
}

// ---------------------------------------------------------------------------
void Help() {
    std::cout << "\n";
    std::cout << "FLAGS                   Experimental flag \n";
    std::cout << "============Service management============\n";
    std::cout << "enable       {SERVICE}     --now / --user \n";
    std::cout << "disable      {SERVICE}     --now / --user \n";
    std::cout << "status       {SERVICE}                    \n";
    std::cout << "restart      {SERVICE}       --user       \n";
    std::cout << "reload       {SERVICE}       --user       \n";
    std::cout << "stop         {SERVICE}       --user       \n";
    std::cout << "start        {SERVICE}       --user       \n";
    std::cout << "is-enabled   {SERVICE}       --user       \n";
    std::cout << "is-active    {SERVICE}     -q / --quiet   \n";
    std::cout << "daemon-reload                              \n";
    std::cout << "list-units                                  \n";
    std::cout << "list-unit-files              --user       \n";
    std::cout << "=============Power management=============\n";
    std::cout << "halt                                       \n";
    std::cout << "poweroff                                   \n";
    std::cout << "reboot                                     \n";
    std::cout << "suspend                                    \n";
    std::cout << "hibernate                                  \n";
    std::cout << "==========================================\n";
}

// ---------------------------------------------------------------------------
int getISActive(const std::string& svc, bool flag) {
    int code = runCommandF("/usr/bin/rc-service", {svc, "status"});

    if (code == 0) {
        if (!flag) std::cout << "active\n";
        return 0;
    }

    if (code > 0) {
        if (!flag) {
            switch (code) {
                case 3:  std::cout << "inactive\n"; break;
                case 1:  std::cout << "failed\n";   break;
                default: std::cout << "unknown status (code: " << code << ")\n"; break;
            }
        }
        return code;
    }

    if (!flag) std::cout << "error: unknown error\n";
    return -1;
}

// ---------------------------------------------------------------------------
std::string getServiceStartTime(const std::string& svc) {
    // 1. Читаем PID из pidfile
    std::string pidfile = "/run/" + svc + ".pid";
    std::ifstream f(pidfile);
    if (!f.is_open()) return "";

    std::string pid;
    std::getline(f, pid);
    // trim справа
    while (!pid.empty() && std::isspace((unsigned char)pid.back())) pid.pop_back();
    if (pid.empty()) return "";

    // 2. Получаем время изменения inode /proc/<pid>
    std::string procPath = "/proc/" + pid;
    struct stat st;
    if (stat(procPath.c_str(), &st) != 0) return "";

    // 3. Преобразуем timestamp в читаемый формат
    time_t t = st.st_ctime;
    struct tm* lt = localtime(&t);
    if (!lt) return "";
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
std::string getServiceRunlevel(const std::string& svc) {
    struct stat st;
    std::string defaultPath = "/etc/runlevels/default/" + svc;
    if (stat(defaultPath.c_str(), &st) == 0) return "default";

    std::string bootPath = "/etc/runlevels/boot/" + svc;
    if (stat(bootPath.c_str(), &st) == 0) return "boot";

    return "";
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: systemctl [OPTIONS...] COMMAND [SERVICE...]\n";
        return 2;
    }

    bool nowFlag   = false;
    bool userFlag  = false;
    bool quietFlag = false;

    // Находим позицию первой НЕ-флаговой команды
    int pos;
    for (pos = 1; pos < argc; pos++) {
        std::string arg = argv[pos];
        if (arg == "--now") {
            nowFlag = true;
        } else if (arg == "--user") {
            userFlag = true;
        } else if (arg == "-q" || arg == "--quiet") {
            quietFlag = true;
        } else if (arg == "-v" || arg == "--verbose") {
            continue;
        } else if (!arg.empty() && arg[0] == '-') {
            // неизвестный флаг — пропускаем (как systemd)
            continue;
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

    // Системные команды — не поддерживают --user
    bool systemOnly =
        command == "halt"            || command == "poweroff" ||
        command == "reboot"          || command == "suspend"  ||
        command == "hibernate"       || command == "list-units" ||
        command == "list-unit-files";

    if (userFlag && systemOnly) {
        std::cerr << "Warning: --user ignored for system command '"
                  << command << "'\n";
        userFlag = false;
    }

    // Вспомогательная функция: добавить --user сразу после имени утилиты
    auto buildCmd = [&](std::vector<std::string> base) -> std::vector<std::string> {
        if (userFlag && !base.empty()) {
            std::vector<std::string> res;
            res.reserve(base.size() + 1);
            res.push_back(base[0]);
            res.push_back("--user");
            for (size_t i = 1; i < base.size(); i++) res.push_back(base[i]);
            return res;
        }
        return base;
    };

    auto runVec = [](const std::vector<std::string>& v) -> int {
        if (v.empty()) return -1;
        std::vector<std::string> args(v.begin() + 1, v.end());
        return runCommand(v[0], args);
    };
    auto runVecF = [](const std::vector<std::string>& v) -> int {
        if (v.empty()) return -1;
        std::vector<std::string> args(v.begin() + 1, v.end());
        return runCommandF(v[0], args);
    };

    // =====================================================================
    if (command == "enable") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& svc : services) {
            runVec(buildCmd({"/usr/bin/rc-update", "add", svc, "default"}));
        }
        if (nowFlag) {
            for (const auto& svc : services) {
                runVec(buildCmd({"/usr/bin/rc-service", svc, "start"}));
            }
        }
        if (!isRoot()) {
            for (const auto& svc : services) {
                runVec(buildCmd({"pkexec", "/usr/bin/rc-update", "add", svc, "default"}));
            }
        }
    }
    // =====================================================================
    else if (command == "disable") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& svc : services) {
            runVec(buildCmd({"/usr/bin/rc-update", "del", svc}));
        }
        if (nowFlag) {
            for (const auto& svc : services) {
                runVec(buildCmd({"/usr/bin/rc-service", svc}));
            }
        }
        if (!isRoot()) {
            for (const auto& svc : services) {
                runVec(buildCmd({"pkexec", "/usr/bin/rc-update", "del", svc}));
            }
        }
    }
    // =====================================================================
    else if (command == "start" || command == "stop" ||
             command == "restart" || command == "reload") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& svc : services) {
            runVecF(buildCmd({"/usr/bin/rc-service", svc, command}));
        }
        if (!isRoot()) {
            for (const auto& svc : services) {
                runVecF(buildCmd({"pkexec", "/usr/bin/rc-service", svc, command}));
            }
        }
    }
    // =====================================================================
    else if (command == "status") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& svc : services) {
            bool enabled = isServiceEnabled(svc, userFlag);

            std::string statusis = "disabled";
            std::string runlevel;
            if (enabled) {
                statusis = "enabled";
                runlevel = getServiceRunlevel(svc);
            }

            int rc = runCommandF("/usr/bin/rc-service", {svc, "status"});
            std::string description = "temporary description";
            std::string startTime   = getServiceStartTime(svc);

            if (rc != 0) {
                std::cout << red << "○ " << reset << " "
                          << svc << ".service - " << description << "\n";
                std::cout << "       Loaded: loaded (/etc/init.d/" << svc
                          << "; " << statusis
                          << "; Runlevel: " << runlevel
                          << "; vendor preset: Plug)\n";
            } else {
                std::cout << green << "● " << reset << " "
                          << svc << ".service - " << description << "\n";
                std::cout << "       Loaded: loaded (/etc/init.d/" << svc
                          << "; " << statusis
                          << "; Runlevel: " << runlevel
                          << "; vendor preset: Plug)\n";
                std::cout << "       Active: active (running) since "
                          << startTime << "\n";
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
        for (const auto& svc : services) {
            bool enabled = isServiceEnabled(svc, userFlag);
            if (enabled) {
                std::cout << svc << " enabled\n";
            } else {
                std::cout << svc << " disabled\n";
                allEnabled = false;
            }
        }
        if (!allEnabled) return 1;
    }
    // =====================================================================
    else if (command == "list-units") {
        runCommand("/usr/bin/rc-status", {"-a"});
    }
    // =====================================================================
    else if (command == "is-active") {
        if (services.empty()) {
            std::cerr << "error: service name required\n";
            return 2;
        }
        for (const auto& svc : services) {
            getISActive(svc, quietFlag);
        }
    }
    // =====================================================================
    else if (command == "daemon-reload") {
        runCommand("/usr/bin/rc-update", {"-u"});
    }
    // =====================================================================
    else if (command == "list-unit-files") {
        if (userFlag) {
            runCommand("/usr/bin/rc-update", {"--user", "show"});
        } else {
            runCommand("/usr/bin/rc-update", {"show"});
        }
    }
    // =====================================================================
    else if (command == "halt") {
        runCommand("halt", {});
    }
    else if (command == "poweroff") {
        runCommand("poweroff", {});
    }
    else if (command == "show") {
        std::cout << "ERR::not release\n";
    }
    else if (command == "reboot") {
        runCommand("reboot", {});
    }
    else if (command == "suspend") {
        runCommand("loginctl", {"suspend"});
    }
    else if (command == "hibernate") {
        runCommand("loginctl", {"hibernate"});
    }
    else if (command == "help") {
        Help();
    }
    // =====================================================================
    else {
        std::cerr << "Unknown command: " << command << "\n";
        return 2;
    }

    return 0;
}
